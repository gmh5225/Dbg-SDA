#include "dec_test_samples.h"
#include "dec_test_functions.h"
#include "decompiler/DecWarningContainer.h"
#include "decompiler/Optimization/DecGraphOptimization.h"
#include "decompiler/PCode/Decoders/DecPCodeDecoderX86.h"
#include "decompiler/PCode/ImageAnalyzer/DecImageAnalyzer.h"
#include "decompiler/SDA/Symbolization/DecGraphSymbolization.h"
#include "images/SimpleImage.h"

void CE::DecTestSamplesPool::Sample::decode() {
	using namespace Decompiler;

	// INSTRUCTION DECODNING (from bytes to pCode graph)
	auto imageGraph = m_imageDec->getPCodeGraph();
	WarningContainer warningContainer;
	RegisterFactoryX86 registerFactoryX86;
	DecoderX86 decoder(&registerFactoryX86, m_imageDec->getInstrPool(), &warningContainer);

	const ImageAnalyzer imageAnalyzer(m_imageDec->getImage(), imageGraph, &decoder, &registerFactoryX86);
	imageAnalyzer.start(m_imageOffset, true);
	m_funcGraph = &*imageGraph->getFunctionGraphList().begin();
}

CE::Function* CE::DecTestSamplesPool::Sample::createFunc(Offset offset, const std::string& name) const {
	auto sig = m_pool->m_project->getTypeManager()->getFactory().createSignature(
		DataType::CallingConvetion::FASTCALL, name + "_sig");
	return m_pool->m_project->getFunctionManager()->getFactory().createFunction(
		offset, sig, m_imageDec, name);
}

CE::DecTestSamplesPool::DecTestSamplesPool(Project* project)
	: m_project(project)
{
	const auto typeFactory = m_project->getTypeManager()->getFactory();
	m_vec3D = typeFactory.createStructure("testVector3D", "");
	m_vec3D->getFields().addField(0x4 * 0, "x", findType("float", ""));
	m_vec3D->getFields().addField(0x4 * 1, "y", findType("float", ""));
	m_vec3D->getFields().addField(0x4 * 2, "z", findType("float", ""));

	m_vecExt3D = typeFactory.createStructure("testVectorExt3D", "");
	m_vecExt3D->getFields().addField(0x8 * 0, "x", findType("float", ""));
	m_vecExt3D->getFields().addField(0x8 * 1, "y", findType("float", ""));
	m_vecExt3D->getFields().addField(0x8 * 2, "z", findType("float", ""));

	m_vec4D = typeFactory.createStructure("testVector4D", "");
	m_vec4D->getFields().addField(0x4 * 0, "x", findType("float", ""));
	m_vec4D->getFields().addField(0x4 * 1, "y", findType("float", ""));
	m_vec4D->getFields().addField(0x4 * 2, "z", findType("float", ""));
	m_vec4D->getFields().addField(0x4 * 3, "w", findType("float", ""));

	m_matrix4x4 = typeFactory.createStructure("testMatrix4x4", "");
	m_matrix4x4->getFields().addField(m_vec4D->getSize() * 0, "vec1", GetUnit(m_vec4D));
	m_matrix4x4->getFields().addField(m_vec4D->getSize() * 1, "vec2", GetUnit(m_vec4D));
	m_matrix4x4->getFields().addField(m_vec4D->getSize() * 2, "vec3", GetUnit(m_vec4D));
	m_matrix4x4->getFields().addField(m_vec4D->getSize() * 3, "vec4", GetUnit(m_vec4D));

	m_defSignature = m_project->getTypeManager()->getFactory().createSignature(
		DataType::CallingConvetion::FASTCALL, "defSignature");
	m_defSignature->addParameter("param1", findType("uint32_t"));
	m_defSignature->addParameter("param2", findType("uint32_t"));
	m_defSignature->addParameter("param3", findType("uint32_t"));
	m_defSignature->addParameter("param4", findType("uint32_t"));
	m_defSignature->addParameter("param5", findType("uint32_t"));
	m_defSignature->setReturnType(findType("uint32_t"));

	m_addrSpace = m_project->getAddrSpaceManager()->createAddressSpace("decompiler samples", "");
}

void CE::DecTestSamplesPool::fillByTests() {
	Sample* sample;
	const auto typeFactory = m_project->getTypeManager()->getFactory();
	const auto symFactory = m_project->getSymbolManager()->getFactory();

	{
		// TEST
		sample = createSampleTest(
			5,
			"test 5",
			"",
			{
				0x48, 0x89, 0x5C, 0x24, 0x08, 0x4C, 0x63, 0x41, 0x14, 0x4C, 0x8B, 0x09, 0x4C, 0x8B, 0xD1, 0x49, 0x2B,
				0xD1, 0x48, 0x8B, 0xC2, 0x48, 0x99, 0x49, 0xF7, 0xF8, 0x48, 0x63,
				0xD8, 0x4C, 0x8B, 0xD8, 0x48, 0x8B, 0xD3, 0x49, 0x0F, 0xAF, 0xD0, 0x42, 0x83, 0x0C, 0x0A, 0xFF, 0x83,
				0x79, 0x18, 0xFF, 0x75, 0x03, 0x89, 0x41, 0x18, 0x83, 0x79, 0x1C, 0xFF,
				0x75, 0x06, 0x48, 0x8D, 0x41, 0x1C, 0xEB, 0x0C, 0x8B, 0x41, 0x14, 0x0F, 0xAF, 0x41, 0x1C, 0x48, 0x98,
				0x48, 0x03, 0x01, 0x44, 0x89, 0x18, 0x48, 0x8B, 0x41, 0x08, 0x44, 0x89,
				0x59, 0x1C, 0x80, 0x0C, 0x18, 0x80, 0x8B, 0x41, 0x20, 0x48, 0x8B, 0x5C, 0x24, 0x08, 0x8D, 0x48, 0xFF,
				0x33, 0xC8, 0x81, 0xE1, 0xFF, 0xFF, 0xFF, 0x3F, 0x33, 0xC8, 0x41, 0x89,
				0x4A, 0x20, 0xC3
			});
	}

	{
		sample = createSampleTest(
			10,
			"simple function",
			"",
			GetFuncBytes(&Test_SimpleFunc));
		auto sig = sample->m_func->getSignature();
		sig->addParameter("a", findType("int32_t", ""));
		sig->addParameter("b", findType("int32_t", ""));
		sig->setReturnType(findType("int32_t"));
	}

	{
		sample = createSampleTest(
			11,
			"multidimension stack array like stackArray[1][2][3]",
			"",
			GetFuncBytes(&Test_Array));
		auto sig = sample->m_func->getSignature();
		sig->setReturnType(findType("uint64_t"));

		sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0x68, findType("int32_t", "[2][3][4]"), "stackArray"), -0x68);
		sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(findType("int64_t", ""), "idx"), 4608);
		sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(findType("uint32_t", ""), "result"), 19201);
	}

	{
		sample = createSampleTest(
			12,
			"hard work with complex data structures",
			"",
			GetFuncBytes(&Test_StructsAndArray));
		auto sig = sample->m_func->getSignature();
		sig->addParameter("myParam1", findType("uint32_t", "[1]"));
		sig->setReturnType(findType("int32_t"));

		sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(findType("uint32_t", "[1]"), "someObject"), 9985);

		sig = sample->createFunc(0xffffffffffffff90, "Func1_2")->getSignature();
		sig->addParameter("param1", findType("uint64_t", ""));
		sig->addParameter("param2", findType("uint64_t", ""));
		sig->setReturnType(findType("uint32_t", "[1]"));
		sample->m_symbolCtx.m_globalSymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(GetUnit(sig), "Func1_2"), 0xffffffffffffff90);
	}

	{
		sample = createSampleTest(
			30,
			"idiv (16 bytes operation)",
			"",
			{
				0x4C, 0x63, 0x41, 0x14, 0x4C, 0x8B, 0x09, 0x4C, 0x8B, 0xD1, 0x49, 0x2B, 0xD1, 0x48, 0x8B, 0xC2, 0x48,
				0x99, 0x49, 0xF7, 0xF8, 0x48, 0x63, 0xD8, 0x4C, 0x8B, 0xD8, 0x48, 0x8B, 0xD3, 0x49, 0x0F, 0xAF, 0xD0,
				0x42, 0x83, 0x0C, 0x0A, 0xFF
			});
	}

	{
		sample = createSampleTest(
			50,
			"xmm registers in incomplete blocks",
			"",
			{
				0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xF8, 0x02, 0x0F, 0x10, 0x44, 0x24, 0x20, 0x75,
				0x05, 0x0F, 0x10, 0x44, 0x24, 0x10, 0x0F, 0x11, 0x44, 0x24, 0x10
			});
	}

	{
		sample = createSampleTest(
			100,
			"entity & coords",
			"get entity and copy his coords to the first param",
			{
				0x40, 0x53, 0x48, 0x83, 0xEC, 0x50, 0x0F, 0x29, 0x74, 0x24, 0x40, 0xF3, 0x0F, 0x10, 0x35, 0x21, 0x46,
				0xF7, 0x01, 0x48, 0x8B, 0xD9, 0x0F, 0x29, 0x7C, 0x24, 0x30, 0xF3, 0x0F, 0x10, 0x3D, 0x15, 0x46, 0xF7,
				0x01, 0x8B, 0xCA, 0x44, 0x0F, 0x29, 0x44, 0x24, 0x20, 0xF3, 0x44, 0x0F, 0x10, 0x05, 0x08, 0x46, 0xF7,
				0x01, 0xE8, 0x5F, 0xE0, 0xFD, 0xFF, 0x48, 0x85, 0xC0, 0x74, 0x14, 0x0F, 0x28, 0x70, 0x70, 0x0F, 0x28,
				0xFE, 0x44, 0x0F, 0x28, 0xC6, 0x0F, 0xC6, 0xFE, 0x55, 0x44, 0x0F, 0xC6, 0xC6, 0xAA, 0xF3, 0x0F, 0x11,
				0x33, 0x0F, 0x28, 0x74, 0x24, 0x40, 0xF3, 0x0F, 0x11, 0x7B, 0x08, 0x0F, 0x28, 0x7C, 0x24, 0x30, 0x48,
				0x8B, 0xC3, 0xF3, 0x44, 0x0F, 0x11, 0x43, 0x10, 0x44, 0x0F, 0x28, 0x44, 0x24, 0x20, 0x48, 0x83, 0xC4,
				0x50, 0x5B, 0xC3, 0x90, 0x48
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("myParam1", findType("uint32_t", "[1]"));
		sig->addParameter("myParam2", findType("uint32_t"));
		sig->setReturnType(findType("uint64_t"));

		{
			auto pos = typeFactory.createStructure("Pos100", "");
			pos->getFields().addField(0x0, "vec", GetUnit(m_vec3D));
			pos->getFields().addField(0xC, "w", findType("uint32_t", ""));
			auto entity = typeFactory.createStructure("Entity100", "");
			entity->getFields().addField(0x70, "pos", GetUnit(pos));

			auto sig = sample->createFunc(0xfffffffffffde098, "getEntitySig")->getSignature();
			sig->addParameter("param1", findType("uint32_t"));
			sig->setReturnType(GetUnit(entity, "[1]"));
			sample->m_symbolCtx.m_globalSymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(GetUnit(sig), "getEntity"), 0xfffffffffffde098);
		}
	}

	{
		sample = createSampleTest(
			101,
			"GET_ANGLE_BETWEEN_2D_VECTORS",
			"",
			{
				0x48, 0x83, 0xEC, 0x38, 0x0F, 0x29, 0x74, 0x24, 0x20, 0x0F, 0x28, 0xF0, 0x0F, 0x28, 0xE1, 0xF3, 0x0F,
				0x59, 0xC9, 0xF3, 0x0F, 0x59, 0xF6, 0xF3, 0x0F, 0x59, 0xE3, 0x0F, 0x28, 0xEA, 0xF3, 0x0F, 0x58, 0xF1,
				0xF3, 0x0F, 0x59, 0xC5, 0x0F, 0x57, 0xD2, 0x0F, 0x2F, 0xF2, 0xF3, 0x0F, 0x58, 0xC4, 0x76, 0x09, 0x0F,
				0x57, 0xE4, 0xF3, 0x0F, 0x51, 0xE6, 0xEB, 0x03, 0x0F, 0x28, 0xE2, 0xF3, 0x0F, 0x59, 0xED, 0xF3, 0x0F,
				0x59, 0xDB, 0xF3, 0x0F, 0x58, 0xEB, 0x0F, 0x2F, 0xEA, 0x76, 0x09, 0x0F, 0x57, 0xC9, 0xF3, 0x0F, 0x51,
				0xCD, 0xEB, 0x03, 0x0F, 0x28, 0xCA, 0xF3, 0x0F, 0x10, 0x1D, 0x59, 0xBF, 0xDF, 0x00, 0xF3, 0x0F, 0x59,
				0xCC, 0xF3, 0x0F, 0x5E, 0xC1, 0x0F, 0x2F, 0xC3, 0x73, 0x03, 0x0F, 0x28, 0xC3, 0xF3, 0x0F, 0x10, 0x0D,
				0xD5, 0xB5, 0xEB, 0x00, 0x0F, 0x2F, 0xC1, 0x76, 0x03, 0x0F, 0x28, 0xC1, 0x0F, 0x2F, 0xC3, 0x76, 0x0F,
				0x0F, 0x2F, 0xC1, 0x73, 0x12, 0xE8, 0x12, 0x4F, 0xCC, 0x00, 0x0F, 0x28, 0xD0, 0xEB, 0x08, 0xF3, 0x0F,
				0x10, 0x15, 0xED, 0x18, 0xE0, 0x00, 0xF3, 0x0F, 0x59, 0x15, 0xF1, 0xBE, 0xDF, 0x00, 0x0F, 0x28, 0x74,
				0x24, 0x20, 0x0F, 0x28, 0xC2, 0x48, 0x83, 0xC4, 0x38, 0xC3
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("X1", findType("float", ""));
		sig->addParameter("Y1", findType("float", ""));
		sig->addParameter("X2", findType("float", ""));
		sig->addParameter("Y2", findType("float", ""));
		sig->setReturnType(findType("float"));

		sample->m_symbolCtx.m_globalSymbolTable->addSymbol(symFactory.createGlobalVarSymbol(0xebb650, findType("float"), "gvar"));
		sig = sample->createFunc(0xcc4fa4, "Func_101")->getSignature();
		sig->setReturnType(findType("float"));
	}

	{
		sample = createSampleTest(
			102,
			"Evklid",
			"",
			{
				0x89, 0xC8, 0x89, 0xD3, 0x83, 0xF8, 0x00, 0x7D, 0x02, 0xF7, 0xD8, 0x83, 0xFB, 0x00, 0x7D, 0x02, 0xF7,
				0xDB, 0x39, 0xD8, 0x7D, 0x01, 0x93, 0x83, 0xFB, 0x00, 0x74, 0x04, 0x29, 0xD8, 0xEB, 0xF2, 0x89, 0x04,
				0x24, 0x89, 0x1C, 0x24
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("a", findType("int32_t", ""));
		sig->addParameter("b", findType("int32_t", ""));
		sig->setReturnType(findType("int32_t"));
	}

	{
		sample = createSampleTest(
			103,
			"JMP function",
			"",
			{
				0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x8B, 0xDA, 0x83, 0xFA, 0x0A, 0x7E, 0x10, 0x8D, 0x42, 0xF5, 0x83,
				0xF8, 0x0D, 0x77, 0x05, 0x83, 0xC3, 0x19, 0xEB, 0x03, 0x83, 0xEB, 0x0E, 0xE8, 0x46, 0xCA, 0xFE, 0xFF,
				0x48, 0x85, 0xC0, 0x74, 0x2C, 0x83, 0xFB, 0x31, 0x77, 0x27, 0x48, 0xBA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0x43, 0x03, 0x00, 0x48, 0x0F, 0xA3, 0xDA, 0x73, 0x17, 0x48, 0x8B, 0x48, 0x48, 0x4C, 0x8B, 0xC0, 0x8B,
				0xD3, 0x48, 0x83, 0xC1, 0x40, 0x48, 0x83, 0xC4, 0x20, 0x5B, 0xE9, 0x0D, 0x10, 0x91, 0xFF, 0x33, 0xC0,
				0x48, 0x83, 0xC4, 0x20, 0x5B, 0xC3, 0xCC
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("param1", findType("int32_t", ""));
		sig->addParameter("param2", findType("int32_t", ""));
		sig->setReturnType(findType("int32_t"));
		sig = sample->createFunc(0xfffffffffffeca68, "Func_103")->getSignature();
		sig->setReturnType(findType("uint64_t"));
	}

	{
		sample = createSampleTest(
			104,
			"RAX request but no EAX",
			"",
			{
				0x48, 0x83, 0xEC, 0x28, 0xE8, 0x1B, 0xB2, 0xFE, 0xFF, 0x48, 0x85, 0xC0, 0x74, 0x0E, 0x48, 0x8B, 0x40,
				0x20, 0x0F, 0xB6, 0x80, 0x18, 0x05, 0x00, 0x00, 0x83, 0xE0, 0x1F, 0x48, 0x83, 0xC4, 0x28, 0xC3, 0x90,
				0x89, 0xED
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("param1", findType("int32_t", ""));
		sig->addParameter("param2", findType("int32_t", ""));
		sig->setReturnType(findType("int32_t"));
		sig = sample->createFunc(0xfffffffffffeb224, "Func_104")->getSignature();
		sig->setReturnType(findType("uint64_t"));
	}

	{
		sample = createSampleTest(
			105,
			"test 105",
			"",
			{
				0x48, 0x83, 0xEC, 0x28, 0x8B, 0x44, 0x24, 0x38, 0x48, 0x8D, 0x54, 0x24, 0x40, 0xC7, 0x44, 0x24, 0x40,
				0xFF, 0xFF, 0x00, 0x00, 0x0D, 0xFF, 0xFF, 0xFF, 0x0F, 0x25, 0xFF, 0xFF, 0xFF, 0x0F, 0x89, 0x44, 0x24,
				0x38, 0xE8, 0x50, 0x8F, 0x8B, 0x00, 0x0F, 0xB7, 0x4C, 0x24, 0x40, 0x66, 0x89, 0x4C, 0x24, 0x38, 0x8B,
				0x4C, 0x24, 0x38, 0x4C, 0x8B, 0xC0, 0x81, 0xC9, 0x00, 0x00, 0xFF, 0x0F, 0x33, 0xC0, 0x0F, 0xBA, 0xF1,
				0x1C, 0x66, 0x81, 0xF9, 0xFF, 0xFF, 0x74, 0x10, 0x4D, 0x85, 0xC0, 0x74, 0x0B, 0x41, 0x0F, 0xB6, 0x80,
				0x18, 0x05, 0x00, 0x00, 0x83, 0xE0, 0x1F, 0x48, 0x83, 0xC4, 0x28, 0xC3, 0xCC, 0x54, 0x48
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("param1", findType("int32_t", ""));
		sig->addParameter("param2", findType("int32_t", ""));
		sig->setReturnType(findType("int32_t"));
		sig = sample->createFunc(0x8b8f78, "Func_105")->getSignature();
		sig->setReturnType(findType("uint64_t"));
	}

	{
		sample = createSampleTest(
			106,
			"matrix vector coords multiplied",
			"",
			{
				0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08, 0x48, 0x89, 0x70, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x60, 0x0F,
				0x29, 0x70, 0xE8, 0xF3, 0x0F, 0x10, 0x35, 0xB4, 0x35, 0xF7, 0x01, 0x0F, 0x29, 0x78, 0xD8, 0xF3, 0x0F,
				0x10, 0x3D, 0xAC, 0x35, 0xF7, 0x01, 0x48, 0x8B, 0xD9, 0x8B, 0xCA, 0x41, 0x8A, 0xF0, 0x44, 0x0F, 0x29,
				0x40, 0xC8, 0x44, 0x0F, 0x29, 0x48, 0xB8, 0xF3, 0x44, 0x0F, 0x10, 0x0D, 0x89, 0x35, 0xF7, 0x01, 0xE8,
				0x1C, 0xD0, 0xFD, 0xFF, 0x48, 0x8B, 0xF8, 0x48, 0x85, 0xC0, 0x0F, 0x84, 0x96, 0x00, 0x00, 0x00, 0x48,
				0x8B, 0x10, 0x48, 0x8B, 0xC8, 0xFF, 0x92, 0x68, 0x03, 0x00, 0x00, 0xF3, 0x44, 0x0F, 0x10, 0x08, 0xF3,
				0x0F, 0x10, 0x70, 0x04, 0xF3, 0x0F, 0x10, 0x78, 0x08, 0x40, 0x84, 0xF6, 0x74, 0x76, 0x48, 0x8B, 0x07,
				0x48, 0x8B, 0xCF, 0xFF, 0x90, 0x68, 0x03, 0x00, 0x00, 0x44, 0x0F, 0x28, 0x47, 0x60, 0x0F, 0x28, 0x7F,
				0x70, 0x0F, 0x28, 0xAF, 0x80, 0x00, 0x00, 0x00, 0x0F, 0x57, 0xF6, 0x66, 0x0F, 0x70, 0x08, 0x00, 0x66,
				0x0F, 0x70, 0x00, 0x55, 0x66, 0x0F, 0x70, 0x18, 0xAA, 0x41, 0x0F, 0x28, 0xE0, 0x0F, 0x28, 0xD7, 0x0F,
				0x15, 0xFE, 0x44, 0x0F, 0x15, 0xC5, 0x0F, 0x14, 0xD6, 0x0F, 0x14, 0xE5, 0x44, 0x0F, 0x14, 0xC7, 0x44,
				0x0F, 0x28, 0xCC, 0x44, 0x0F, 0x15, 0xCA, 0x0F, 0x14, 0xE2, 0x44, 0x0F, 0x59, 0xC3, 0x44, 0x0F, 0x59,
				0xC8, 0x0F, 0x59, 0xE1, 0x44, 0x0F, 0x58, 0xCC, 0x45, 0x0F, 0x58, 0xC8, 0x41, 0x0F, 0x28, 0xF1, 0x41,
				0x0F, 0x28, 0xF9, 0x41, 0x0F, 0xC6, 0xF1, 0x55, 0x41, 0x0F, 0xC6, 0xF9, 0xAA, 0x48, 0x8B, 0x74, 0x24,
				0x78, 0x44, 0x0F, 0x28, 0x44, 0x24, 0x30, 0xF3, 0x44, 0x0F, 0x11, 0x0B, 0x44, 0x0F, 0x28, 0x4C, 0x24,
				0x20, 0xF3, 0x0F, 0x11, 0x73, 0x08, 0x0F, 0x28, 0x74, 0x24, 0x50, 0xF3, 0x0F, 0x11, 0x7B, 0x10, 0x48,
				0x8B, 0xC3, 0x48, 0x8B, 0x5C, 0x24, 0x70, 0x0F, 0x28, 0x7C, 0x24, 0x40, 0x48, 0x83, 0xC4, 0x60, 0x5F,
				0xC3
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("param1", GetUnit(m_vecExt3D, "[1]"));
		sig->addParameter("param2", findType("int32_t", ""));
		sig->addParameter("param3", findType("bool", ""));
		sig->setReturnType(GetUnit(m_vecExt3D, "[1]"));
		sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(GetUnit(m_vec3D, "[1]"), "pos2"), 23042);
		sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(GetUnit(m_vec3D, "[1]"), "pos3"), 31234);

		{
			auto vtable = typeFactory.createStructure("Vtable106", "");
			vtable->getFields().addField(0x368, "getPos", findType("void", "[1]"));

			auto EntityTypeEnum = typeFactory.createEnum("EntityType", "");
			EntityTypeEnum->addField("ENTITY_PLAYER", 1);
			EntityTypeEnum->addField("ENTITY_PED", 2);
			EntityTypeEnum->addField("ENTITY_VEHICLE", 3);

			auto entity = typeFactory.createStructure("Entity106", "");
			entity->getFields().addField(0, "vtable", GetUnit(vtable, "[1]"));
			entity->getFields().addField(8, "type", GetUnit(EntityTypeEnum));
			entity->getFields().addField(96, "matrix", GetUnit(m_matrix4x4));
			sample->m_symbolCtx.m_funcBodySymbolTable->addSymbol(symFactory.createLocalInstrVarSymbol(GetUnit(entity, "[1]"), "entity"), 17152);
		}

		sig = sample->createFunc(0xfffffffffffdd064, "GetEntity")->getSignature();
		sig->setReturnType(findType("uint64_t"));
		sample->m_imageDec->getVirtFuncCalls()[ComplexOffset(0x5A, 2)] = sig;
		sample->m_imageDec->getVirtFuncCalls()[ComplexOffset(0x7A, 2)] = sig;
	}

	{
		sample = createSampleTest(
			107,
			"hard stack memory copying",
			"",
			{
				0x40, 0x55, 0x48, 0x8D, 0x6C, 0x24, 0xA9, 0x48, 0x81, 0xEC, 0xD0, 0x00, 0x00, 0x00, 0xF2, 0x0F, 0x10,
				0x4D, 0xC7, 0x45, 0x33, 0xC0, 0x48, 0x8D, 0x45, 0x17, 0x44, 0x89, 0x45, 0xBF, 0xF2, 0x0F, 0x11, 0x4D,
				0x27, 0xF2, 0x0F, 0x10, 0x4D, 0xC7, 0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x8D, 0x45, 0xD7, 0x4C, 0x8D,
				0x4D, 0xF7, 0x0F, 0x10, 0x45, 0xB7, 0xF2, 0x0F, 0x11, 0x4D, 0xE7, 0xF2, 0x0F, 0x10, 0x4D, 0xC7, 0x44,
				0x89, 0x45, 0xBF, 0x48, 0x8D, 0x15, 0x5E, 0xC5, 0x6C, 0x01, 0x48, 0x89, 0x44, 0x24, 0x20, 0x0F, 0x29,
				0x45, 0x17, 0x0F, 0x10, 0x45, 0xB7, 0xF2, 0x0F, 0x11, 0x4D, 0x07, 0xF2, 0x0F, 0x10, 0x4D, 0xC7, 0x44,
				0x89, 0x45, 0xBF, 0x4C, 0x8D, 0x45, 0x37, 0x0F, 0x29, 0x45, 0xD7, 0x0F, 0x10, 0x45, 0xB7, 0xC7, 0x45,
				0xBF, 0x01, 0x00, 0x00, 0x00, 0xF2, 0x0F, 0x11, 0x4D, 0x47, 0x0F, 0x29, 0x45, 0xF7, 0x66, 0x0F, 0x6E,
				0xC1, 0x48, 0x8D, 0x0D, 0xC8, 0xF4, 0xE2, 0x01, 0xF3, 0x0F, 0xE6, 0xC0, 0xF2, 0x0F, 0x11, 0x45, 0xB7,
				0x0F, 0x10, 0x45, 0xB7, 0x0F, 0x29, 0x45, 0x37, 0xE8, 0xAA, 0xDE, 0xFE, 0xFF, 0x48, 0x81, 0xC4, 0xD0,
				0x00, 0x00, 0x00, 0x5D, 0xC3, 0xCC
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("param1", findType("int32_t", ""));

		{
			auto valueUI = typeFactory.createStructure("ValueUI107", "");
			valueUI->getFields().addField(0x0, "m_value", findType("uint64_t", ""));
			valueUI->getFields().addField(0x8, "m_type", findType("uint32_t", ""));
			valueUI->getFields().addField(0xC, "m_unk", findType("uint32_t", ""));
			valueUI->getFields().addField(0x10, "m_formatText", findType("uint64_t", ""));
			//todo: test this sample without these definitions
			sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0xA8, GetUnit(valueUI), "value1"), -0xA8);
			sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0x88, GetUnit(valueUI), "value2"), -0x88);
			sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0x68, GetUnit(valueUI), "value3"), -0x68);
			sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0x48, GetUnit(valueUI), "value4"), -0x48);
			sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-0x28, GetUnit(valueUI), "value5"), -0x28);

			auto uiDrawSig = sample->createFunc(0xfffffffffffedf50, "UI_Draw107")->getSignature();
			uiDrawSig->addParameter("param1", findType("uint64_t", ""));
			uiDrawSig->addParameter("param2", findType("uint64_t", ""));
			uiDrawSig->addParameter("param3", GetUnit(valueUI, "[1]"));
			uiDrawSig->addParameter("param4", GetUnit(valueUI, "[1]"));
			uiDrawSig->addParameter("param5", GetUnit(valueUI, "[1]"));
			uiDrawSig->addParameter("param6", GetUnit(valueUI, "[1]"));

			// todo: *(uint64_t*)&value1 = (uint64_t)(float)(param1)		->		 *(float*)&value1 = (float)(param1)
		}
	}

	{
		sample = createSampleTest(
			108,
			"Matrix_FillWithVectorsAndMul",
			"",
			{
				0x4C, 0x8B, 0xDC, 0x48, 0x81, 0xEC, 0xB8, 0x00, 0x00, 0x00, 0x0F, 0x28, 0x02, 0x48, 0x8B, 0x94, 0x24,
				0xF0, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x84, 0x24, 0xE0, 0x00, 0x00, 0x00, 0x41, 0x0F, 0x29, 0x73, 0xE8,
				0x41, 0x0F, 0x29, 0x7B, 0xD8, 0x45, 0x0F, 0x29, 0x43, 0xC8, 0x4D, 0x8D, 0x1B, 0x66, 0x0F, 0x70, 0x22,
				0x55, 0x66, 0x0F, 0x70, 0x32, 0xAA, 0x66, 0x0F, 0x70, 0x2A, 0x00, 0x45, 0x0F, 0x29, 0x4B, 0xB8, 0x45,
				0x0F, 0x29, 0x53, 0xA8, 0x45, 0x0F, 0x29, 0x5B, 0x98, 0x45, 0x0F, 0x29, 0x63, 0x88, 0x44, 0x0F, 0x29,
				0x6C, 0x24, 0x30, 0x44, 0x0F, 0x28, 0x28, 0x48, 0x8B, 0x84, 0x24, 0xE8, 0x00, 0x00, 0x00, 0x66, 0x0F,
				0x70, 0x08, 0x00, 0x66, 0x0F, 0x70, 0x18, 0xAA, 0x44, 0x0F, 0x29, 0x74, 0x24, 0x20, 0x45, 0x0F, 0x28,
				0x30, 0x44, 0x0F, 0x29, 0x7C, 0x24, 0x10, 0x4C, 0x8B, 0x84, 0x24, 0xF8, 0x00, 0x00, 0x00, 0x66, 0x45,
				0x0F, 0x70, 0x00, 0x00, 0x66, 0x41, 0x0F, 0x70, 0x38, 0x55, 0x66, 0x45, 0x0F, 0x70, 0x08, 0xAA, 0x45,
				0x0F, 0x28, 0x39, 0x0F, 0x29, 0x04, 0x24, 0x41, 0x0F, 0x28, 0xD6, 0x4C, 0x8B, 0x8C, 0x24, 0x00, 0x01,
				0x00, 0x00, 0x66, 0x0F, 0x70, 0x00, 0x55, 0x66, 0x45, 0x0F, 0x70, 0x11, 0x00, 0x66, 0x45, 0x0F, 0x70,
				0x19, 0x55, 0x0F, 0x59, 0xD0, 0x0F, 0x28, 0x04, 0x24, 0x0F, 0x59, 0xC1, 0x41, 0x0F, 0x28, 0xCF, 0x0F,
				0x59, 0xCB, 0x66, 0x45, 0x0F, 0x70, 0x21, 0xAA, 0x0F, 0x58, 0xD0, 0x41, 0x0F, 0x28, 0xDE, 0x0F, 0x59,
				0xDC, 0x0F, 0x28, 0x24, 0x24, 0x0F, 0x28, 0xC4, 0x0F, 0x58, 0xD1, 0x0F, 0x59, 0xC5, 0x41, 0x0F, 0x28,
				0xCF, 0x0F, 0x59, 0xCE, 0x0F, 0x58, 0xD8, 0x41, 0x0F, 0x28, 0x73, 0xE8, 0x0F, 0x28, 0xC4, 0x0F, 0x29,
				0x11, 0x41, 0x0F, 0x28, 0xD6, 0x41, 0x0F, 0x59, 0xE2, 0x45, 0x0F, 0x59, 0xF3, 0x0F, 0x58, 0xD9, 0x41,
				0x0F, 0x58, 0xE5, 0x45, 0x0F, 0x28, 0x53, 0xA8, 0x45, 0x0F, 0x28, 0x5B, 0x98, 0x44, 0x0F, 0x28, 0x6C,
				0x24, 0x30, 0x0F, 0x59, 0xD7, 0x41, 0x0F, 0x59, 0xC0, 0x41, 0x0F, 0x58, 0xE6, 0x0F, 0x58, 0xD0, 0x41,
				0x0F, 0x28, 0x7B, 0xD8, 0x45, 0x0F, 0x28, 0x43, 0xC8, 0x44, 0x0F, 0x28, 0x74, 0x24, 0x20, 0x41, 0x0F,
				0x28, 0xCF, 0x0F, 0x29, 0x59, 0x10, 0x45, 0x0F, 0x59, 0xFC, 0x41, 0x0F, 0x59, 0xC9, 0x45, 0x0F, 0x28,
				0x4B, 0xB8, 0x45, 0x0F, 0x28, 0x63, 0x88, 0x41, 0x0F, 0x58, 0xE7, 0x0F, 0x58, 0xD1, 0x44, 0x0F, 0x28,
				0x7C, 0x24, 0x10, 0x0F, 0x29, 0x61, 0x30, 0x0F, 0x29, 0x51, 0x20, 0x49, 0x8B, 0xE3, 0xC3
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("pOutMatrix", GetUnit(m_matrix4x4, "[1]"));
		sig->addParameter("leftVec1", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("upVec1", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("forwardVec1", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("translationVec1", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("leftVec2", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("upVec2", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("forwardVec2", GetUnit(m_vec4D, "[1]"));
		sig->addParameter("translationVec2", GetUnit(m_vec4D, "[1]"));
		sig->setReturnType(findType("uint64_t"));

		sample->m_symbolCtx.m_stackSymbolTable->addSymbol(symFactory.createLocalStackVarSymbol(-static_cast<int>(0xffffffd0), GetUnit(m_matrix4x4, "[1]"), "matrix"), -static_cast<int>(0xffffffd0));
	}

	{
		/*
			TODO:
			- ��������� backOrderId
			- ��������� localVarb2b: �� �� ������ ���� ������, � ������������ �����, ���� � ��� � ����������

			1. ������� �������� ����������� (�� �������� �������� ������� � ������)
				global_0x13c0c28 = 2120794248
				localVar1b3c = 2120794248 -> global_0x13c0c28
			2. memVar1c59 = *(uint64_t*)(funcVarff9 +.8 0x10)
			3. ������ goto, ������ �������� ����� (����� ������������� ���� �����)
		*/

		sample = createSampleTest(
			109,
			"SET_ENTITY_ANIM_SPEED",
			"",
			{
				0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08, 0x48, 0x89, 0x70, 0x10, 0x48, 0x89, 0x78, 0x18, 0x4C, 0x89,
				0x70, 0x20, 0x55, 0x48, 0x8B, 0xEC, 0x48, 0x83, 0xEC, 0x50, 0x0F, 0x29, 0x70, 0xE8, 0x4C, 0x8D, 0x0D,
				0x9A, 0xBA, 0xE9, 0x00, 0x49, 0x8B, 0xF0, 0x0F, 0x28, 0xF3, 0x4C, 0x8B, 0xF2, 0x8B, 0xF9, 0xE8, 0x3E,
				0xCD, 0x00, 0x00, 0x4C, 0x8B, 0xC8, 0x48, 0x85, 0xC0, 0x74, 0x5A, 0x48, 0x8B, 0x40, 0x10, 0x48, 0x85,
				0xC0, 0x74, 0x0E, 0x8A, 0x88, 0x14, 0x02, 0x00, 0x00, 0xC0, 0xE9, 0x06, 0x80, 0xE1, 0x01, 0xEB, 0x02,
				0x32, 0xC9, 0x84, 0xC9, 0x75, 0x3D, 0x8B, 0x05, 0xCB, 0x0B, 0x3C, 0x01, 0xA8, 0x01, 0x75, 0x16, 0x83,
				0xC8, 0x01, 0x89, 0x05, 0xBE, 0x0B, 0x3C, 0x01, 0xB8, 0x88, 0xC0, 0x68, 0x7E, 0x89, 0x05, 0xAF, 0x0B,
				0x3C, 0x01, 0xEB, 0x06, 0x8B, 0x05, 0xA7, 0x0B, 0x3C, 0x01, 0x48, 0x8D, 0x55, 0xE0, 0x0F, 0x28, 0xD6,
				0x49, 0x8B, 0xC9, 0x89, 0x45, 0xE0, 0xE8, 0xE9, 0x9A, 0xE1, 0xFF, 0xE9, 0xC8, 0x00, 0x00, 0x00, 0x41,
				0xB0, 0x01, 0x41, 0xB9, 0x07, 0x00, 0x00, 0x00, 0x8B, 0xCF, 0x41, 0x8A, 0xD0, 0xE8, 0x5D, 0xC1, 0x00,
				0x00, 0x48, 0x8B, 0xD8, 0x48, 0x85, 0xC0, 0x74, 0x45, 0x48, 0x8B, 0xD6, 0x33, 0xC9, 0xE8, 0x47, 0x17,
				0x7E, 0x00, 0x49, 0x8B, 0xD6, 0x33, 0xC9, 0x89, 0x45, 0xE0, 0xE8, 0x3A, 0x17, 0x7E, 0x00, 0x4C, 0x8D,
				0x4D, 0xEC, 0x89, 0x45, 0xE4, 0x48, 0x8D, 0x45, 0xE8, 0x4C, 0x8D, 0x45, 0xE0, 0x48, 0x8D, 0x55, 0xE4,
				0x48, 0x8B, 0xCB, 0x48, 0x89, 0x44, 0x24, 0x20, 0xE8, 0x62, 0xC0, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x0A,
				0x44, 0x8A, 0x4D, 0xE8, 0x44, 0x8B, 0x45, 0xEC, 0xEB, 0x5D, 0x41, 0xB9, 0x07, 0x00, 0x00, 0x00, 0x45,
				0x33, 0xC0, 0xB2, 0x01, 0x8B, 0xCF, 0xE8, 0xFE, 0xC0, 0x00, 0x00, 0x48, 0x8B, 0xD8, 0x48, 0x85, 0xC0,
				0x74, 0x4E, 0x48, 0x8B, 0xD6, 0x33, 0xC9, 0xE8, 0xE8, 0x16, 0x7E, 0x00, 0x49, 0x8B, 0xD6, 0x33, 0xC9,
				0x89, 0x45, 0xEC, 0xE8, 0xDB, 0x16, 0x7E, 0x00, 0x4C, 0x8D, 0x4D, 0xE0, 0x89, 0x45, 0xE8, 0x48, 0x8D,
				0x45, 0xE4, 0x4C, 0x8D, 0x45, 0xEC, 0x48, 0x8D, 0x55, 0xE8, 0x48, 0x8B, 0xCB, 0x48, 0x89, 0x44, 0x24,
				0x20, 0xE8, 0x03, 0xC0, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x13, 0x44, 0x8A, 0x4D, 0xE4, 0x44, 0x8B, 0x45,
				0xE0, 0x0F, 0x28, 0xCE, 0x48, 0x8B, 0xCB, 0xE8, 0x74, 0x69, 0x8E, 0xFF, 0x48, 0x8B, 0x5C, 0x24, 0x60,
				0x48, 0x8B, 0x74, 0x24, 0x68, 0x48, 0x8B, 0x7C, 0x24, 0x70, 0x0F, 0x28, 0x74, 0x24, 0x40, 0x4C, 0x8B,
				0x74, 0x24, 0x78, 0x48, 0x83, 0xC4, 0x50, 0x5D, 0xC3
			});
		auto sig = sample->m_func->getSignature();
		sig->addParameter("p1_Entity", findType("int32_t", ""));
		sig->addParameter("p2_AnimDict", findType("char", "[1]"));
		sig->addParameter("p2_AnimName", findType("char", "[1]"));
		sig->addParameter("p4_Speed", findType("float", ""));

		{
			sig = sample->createFunc(0xffffffffffe19b7c, "Func0_109")->getSignature();
			sig->addParameter("param1", findType("uint64_t", ""));
			sig->addParameter("param2", findType("uint64_t", ""));
			sig->addParameter("param3", findType("uint64_t", ""));
			sig->addParameter("param4", findType("uint64_t", ""));
			sig->setReturnType(findType("bool"));

			sig = sample->createFunc(0xc14c, "Func1_109")->getSignature();
			sig->addParameter("param1", findType("uint64_t", ""));
			sig->addParameter("param2", findType("uint64_t", ""));
			sig->addParameter("param3", findType("uint64_t", ""));
			sig->addParameter("param4", findType("uint64_t", ""));
			sig->setReturnType(findType("bool"));

			sig = sample->createFunc(0xcd74, "Func2_109")->getSignature();
			sig->addParameter("param1", findType("uint32_t", ""));
			sig->addParameter("param2", findType("char", "[1]"));
			sig->addParameter("param3", findType("char", "[1]"));
			sig->setReturnType(findType("uint64_t"));

			sig = sample->createFunc(0xc208, "Func4_109")->getSignature();
			sig->addParameter("param1", findType("uint32_t", ""));
			sig->addParameter("param2", findType("bool", ""));
			sig->addParameter("param3", findType("bool", ""));
			sig->addParameter("param4", findType("uint32_t", ""));
			sig->setReturnType(findType("uint64_t"));

			sig = sample->createFunc(0x7e1804, "Func5_109")->getSignature();
			sig->addParameter("param1", findType("uint32_t", ""));
			sig->addParameter("param2", findType("uint64_t", ""));
			sig->addParameter("param3", findType("bool", ""));
			sig->addParameter("param4", findType("uint32_t", ""));
			sig->setReturnType(findType("uint32_t"));

			sig = sample->createFunc(0xffffffffff8e6ad4, "Func6_109")->getSignature();
			sig->addParameter("param1", findType("uint64_t", ""));
			sig->addParameter("param2", findType("uint64_t", ""));
			sig->addParameter("param3", findType("uint32_t", ""));
			sig->addParameter("param4", findType("byte", ""));
		}
	}
}

void CE::DecTestSamplesPool::analyze() {
	for (auto& sample : m_samples) {
		sample.decode();
	}
}

CE::DecTestSamplesPool::Sample* CE::DecTestSamplesPool::createSampleTest(int testId, const std::string& name,
                                                                         const std::string& comment, AbstractImage* image, int offset) {
	const auto suffix = std::to_string(testId);
	Sample sample;
	sample.m_testId = testId;
	sample.m_name = name;
	sample.m_comment = comment;
	sample.m_imageDec = m_project->getImageManager()->createImage(m_addrSpace, ImageDecorator::IMAGE_PE, suffix + ": " + name, comment);
	sample.m_imageDec->setImage(image);
	sample.m_imageOffset = offset;
	sample.m_pool = this;

	auto sig = m_project->getTypeManager()->getFactory().createSignature(
		DataType::CallingConvetion::FASTCALL, "sig_" + suffix);
	sample.m_func = m_project->getFunctionManager()->getFactory().createFunction(
		0x0, sig, sample.m_imageDec, "func_" + suffix);
	sample.m_symbolCtx = sample.m_func->getSymbolContext();
	m_samples.push_back(sample);
	return &*m_samples.rbegin();
}

CE::DecTestSamplesPool::Sample* CE::DecTestSamplesPool::createSampleTest(int testId, const std::string& name,
                                                                         const std::string& comment,
                                                                         std::vector<uint8_t> content) {
	return createSampleTest(testId, name, comment, new SimpleImage(new VectorReader(content)));
}

CE::DataTypePtr CE::DecTestSamplesPool::findType(std::string typeName, std::string typeLevel) const {
	return GetUnit(m_project->getTypeManager()->findTypeByName(typeName), typeLevel);
}

int CE::DecTestSamplesPool::CalculateFuncSize(uint8_t* addr, bool endByRet) {
	int size = 0;
	while (!(addr[size] == 0xC3 && addr[size + 1] == 0xCC))
		size++;
	return size + 1;
}

std::vector<uint8_t> CE::DecTestSamplesPool::GetFuncBytes(void* addr) {
	const auto size = std::min(CalculateFuncSize(static_cast<uint8_t*>(addr), 0), 0x1000);
	return std::vector(static_cast<uint8_t*>(addr), static_cast<uint8_t*>(addr) + size);
}
