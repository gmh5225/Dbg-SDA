#include <iostream>
#include "Plugin/PluginLoader.h"
#include "Program.h"
#include "Project.h"
#include "Factory.h"
#include "Core/Context.h"
#include "Core/DataType/VoidDataType.h"
#include "Core/DataType/PointerDataType.h"
#include "Core/DataType/ScalarDataType.h"
#include "Core/DataType/TypedefDataType.h"
#include "Core/DataType/StructureDataType.h"
#include "Core/DataType/DataTypeParser.h"
#include "Core/Image/AddressSpace.h"
#include "Core/Image/Image.h"
#include "Core/SymbolTable/StandartSymbolTable.h"
#include "Core/SymbolTable/SymbolTableParser.h"
#include "Core/Pcode/PcodeParser.h"
#include "Core/Pcode/PcodeRender.h"
#include "Core/IRcode/IRcodeRender.h"
#include "Database/Database.h"
#include "Database/Schema.h"
#include "Database/Loader.h"
#include "Database/Transaction.h"
#include "Callbacks/ContextCallbacks.h"
#include "Change.h"
#include "Platform/X86/PcodeDecoder.h"
#include "Platform/X86/InstructionDecoder.h"
#include "Platform/X86/RegisterHelper.h"
#include "Platform/X86/CallingConvention.h"
#include "Decompiler/Pcode/PcodeGraphBuilder.h"
#include "Decompiler/Pcode/VtableLookup.h"
#include "Decompiler/IRcode/Generator/IRcodeBlockGenerator.h"
#include "Decompiler/Semantics/SemanticsManager.h"
#include <boost/functional/hash.hpp>

using namespace sda;

void testPcodeDecoder() {
    auto ctx = new Context();
    auto image = new Image(
        ctx,
        std::make_unique<VectorImageRW>(
            std::vector<uint8_t>({
				0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xF8, 0x02, 0x0F, 0x10, 0x44, 0x24, 0x20, 0x75,
				0x05, 0x0F, 0x10, 0x44, 0x24, 0x10, 0x0F, 0x11, 0x44, 0x24, 0x10
			})
        ),
        std::make_shared<TestAnalyser>(),
        nullptr,
        "xmm registers in incomplete blocks"
    );


    // P-code decoder
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    platform::PcodeDecoderX86 pcodeDecoder(&decoder);
    platform::InstructionDecoderX86 instrDecoder(&decoder);
    platform::RegisterHelperX86 regHelper;
    
    auto offset = image->getEntryPointOffset();

    pcode::StreamRender pcodeRender(std::cout, &regHelper);
    Instruction::StreamRender srcInstrRender(std::cout);
    while (offset < image->getSize()) {
        std::vector<uint8_t> data(0x100);
        image->getRW()->readBytesAtOffset(offset, data);
        pcodeDecoder.decode(offset, data);
        
        instrDecoder.decode(data);
        srcInstrRender.render(instrDecoder.getDecodedInstruction());
        std::cout << std::endl;
        auto decodedInstructions = pcodeDecoder.getDecodedInstructions();
        for (auto& decodedInstruction : decodedInstructions) {
            std::cout << "    ";
            pcodeRender.renderInstruction(&decodedInstruction);
            std::cout << std::endl;
        }

        offset += pcodeDecoder.getInstructionLength();
    }



    // P-code graph builder
    decompiler::PcodeGraphBuilder graphBuilder(image->getPcodeGraph(), image, &pcodeDecoder);

    class GraphCallbacks : public pcode::Graph::Callbacks {
    public:
        std::shared_ptr<pcode::Graph::Callbacks> m_otherCallbacks;

        void onFunctionGraphCreated(pcode::FunctionGraph* functionGraph) override {
            // create function symbol here
            std::cout << "Function graph created: " << functionGraph->getEntryBlock()->getMinOffset() << std::endl;
            m_otherCallbacks->onFunctionGraphCreated(functionGraph);
        }

        void onFunctionGraphRemoved(pcode::FunctionGraph* functionGraph) override {
            std::cout << "Function graph removed: " << functionGraph->getEntryBlock()->getMinOffset() << std::endl;
            m_otherCallbacks->onFunctionGraphRemoved(functionGraph);
        }
    };
    auto graphCallbacks = std::make_shared<GraphCallbacks>();
    graphCallbacks->m_otherCallbacks = image->getPcodeGraph()->getCallbacks();
    image->getPcodeGraph()->setCallbacks(graphCallbacks);

    // remove old instructions
    // image->getPcodeGraph()->removeInstruction();

    // reanalyse
    auto funcCallLookupCallbacks = std::make_shared<decompiler::VtableLookupCallbacks>(
        image, graphBuilder.getBlockBuilder());
    graphBuilder.getBlockBuilder()->setCallbacks(funcCallLookupCallbacks);
    graphBuilder.start({ pcode::InstructionOffset(image->getEntryPointOffset(), 0) }, true);

    image->getPcodeGraph()->setCallbacks(graphCallbacks->m_otherCallbacks);
}

void initDefaultDataTypes(Context* ctx) {
    // base data types
    ScalarDataType* scalars[] = {
        new ScalarDataType(ctx, nullptr, "uint8_t", ScalarType::UnsignedInt, 1),
        new ScalarDataType(ctx, nullptr, "int8_t", ScalarType::SignedInt, 1),
        new ScalarDataType(ctx, nullptr, "uint16_t", ScalarType::UnsignedInt, 2),
        new ScalarDataType(ctx, nullptr, "int16_t", ScalarType::SignedInt, 2),
        new ScalarDataType(ctx, nullptr, "uint32_t", ScalarType::UnsignedInt, 4),
        new ScalarDataType(ctx, nullptr, "int32_t", ScalarType::SignedInt, 4),
        new ScalarDataType(ctx, nullptr, "uint64_t", ScalarType::UnsignedInt, 8),
        new ScalarDataType(ctx, nullptr, "int64_t", ScalarType::SignedInt, 8),
        new ScalarDataType(ctx, nullptr, "float", ScalarType::FloatingPoint, 4),
        new ScalarDataType(ctx, nullptr, "double", ScalarType::FloatingPoint, 8)
    };
    new VoidDataType(ctx, nullptr);

    // derived data types
    auto dataTypesStr = "\
        bool = typedef uint8_t \
    ";
    DataTypeParser::Parse(dataTypesStr, ctx);
}

void testDecompiler() {
    auto pcodeStr = "\
        rcx:8 = COPY rcx:8 \
        rbx:8 = INT_MULT rdx:8, 4:8 \
        rbx:8 = INT_ADD rcx:8, rbx:8 \
        rbx:8 = INT_ADD rbx:8, 0x10:8 \
        STORE rbx:8, 1.0:8 \
    ";

    platform::RegisterHelperX86 regHelper;

    pcode::StreamRender pcodeRender(std::cout, &regHelper);

    auto pcodeInstructions = pcode::Parser::Parse(pcodeStr, &regHelper);
    for (auto& instr : pcodeInstructions) {
        std::cout << "    ";
        pcodeRender.renderInstruction(&instr);
        std::cout << std::endl;
    }

    using namespace decompiler;

    auto ctx = new Context();
    initDefaultDataTypes(ctx);
    auto symbolTableStr = "{}";
    auto globalSymbolTable = SymbolTableParser::Parse(symbolTableStr, ctx);
    
    auto dataTypesStr = "\
        EntityType = enum { \
            VEHICLE, \
            PED \
        } \
        \
        ['entity can be vehicle or player'] \
        Entity = struct { \
            EntityType type, \
            float x = 0x10, \
            float y \
        } \
        \
        SetEntityVelAxisSig = signature fastcall void(Entity* entity, uint32_t idx) \
    ";
    auto parsedDt = DataTypeParser::Parse(dataTypesStr, ctx, { std::make_shared<platform::FastcallCallingConvention>() });
    auto functionSignature = dynamic_cast<SignatureDataType*>(parsedDt["SetEntityVelAxisSig"]);
    auto functionSymbol = new FunctionSymbol(ctx, nullptr, "main", functionSignature);

    SemanticsManager semManager(ctx);
    new BaseSemanticsPropagator(&semManager);
    auto semCtx = std::make_shared<SemanticsContext>();
    semCtx->globalSymbolTable = globalSymbolTable;
    semCtx->functionSymbol = functionSymbol;

    pcode::Block pcodeBlock;
    ircode::Block ircodeBlock(&pcodeBlock);
    TotalMemorySpace memorySpace;
    IRcodeDataTypeProvider dataTypeProvider(ctx);
    IRcodeBlockGenerator ircodeGen(&ircodeBlock, &memorySpace, &dataTypeProvider);

    ircode::StreamRender ircodeRender(std::cout, &pcodeRender);
    ircodeRender.setExtendInfo(true);

    for (const auto& pcodeInstruction : pcodeInstructions) {
        ircodeGen.executePcode(&pcodeInstruction);
        for (auto ircodeOp : ircodeGen.getGeneratedOperations()) {
            std::cout << "    ";
            ircodeRender.renderOperation(ircodeOp);
            std::cout << std::endl;

            SemanticsContextOperations ctxOps;
            ctxOps.insert({ semCtx, ircodeOp });
            semManager.propagate(ctxOps);
        }
    }

    std::cout << std::endl;
}

void testGeneral() {
    Program program;
    for (auto plugin : GetPlugins("plugins")) {
        plugin->onPluginLoaded(&program);
        program.addPlugin(std::unique_ptr<IPlugin>(plugin.get()));
    }

    auto ctx = new Context();
    auto project = new Project(&program, "Test", ctx);
    auto ctxCallbacks = new ContextCallbacks(ctx, project);
    project->getDatabase()->init();

    project->getChangeChain()->newChangeList();
    auto space1 = new AddressSpace(ctx, nullptr, "space1");
    space1->setName("space1");
    auto space2 = new AddressSpace(ctx, nullptr, "space2");
    project->getTransaction()->commit();

    project->getChangeChain()->undo();

    auto ctx2 = new Context();
    auto oldCallbacks = ctx2->getCallbacks();
    ctx2->setCallbacks(std::make_shared<Context::Callbacks>());
    Factory factory(ctx2);
    Loader loader(project->getDatabase(), &factory);
    loader.load();
    ctx2->setCallbacks(oldCallbacks);

    int a = 5;
}

int main(int argc, char *argv[])
{
    //testPcodeDecoder();
    testDecompiler();
    //testGeneral();
    return 0;
}