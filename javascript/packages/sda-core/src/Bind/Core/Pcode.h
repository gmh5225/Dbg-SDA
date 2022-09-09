#pragma once
#include "Core/Pcode/PcodeInstruction.h"
#include "Core/Pcode/PcodeParser.h"
#include "Core/Pcode/PcodePrinter.h"

namespace sda::bind
{
    class PcodeVarnodeBind
    {
    public:
        static void Init(v8pp::module& module) {
            v8pp::class_<pcode::Varnode, v8pp::shared_ptr_traits> cl(module.isolate());
            cl
                .property("size", &pcode::Varnode::getSize)
                .property("isRegister", &pcode::Varnode::isRegister);
            module.class_("PcodeVarnode", cl);
        }
    };

    class PcodeInstructionBind
    {
    public:
        static void Init(v8pp::module& module) {
            v8pp::class_<pcode::Instruction> cl(module.isolate());
            cl
                .auto_wrap_objects(true)
                .property("id", &pcode::Instruction::getId)
                .property("input0", &pcode::Instruction::getInput0)
                .property("input1", &pcode::Instruction::getInput1)
                .property("output", &pcode::Instruction::getOutput)
                .property("offset", &pcode::Instruction::getOffset);
            module.class_("PcodeInstruction", cl);
        }
    };

    class PcodeParserBind
    {
        static auto Parse(const std::string& text, const RegisterRepository* regRepo) {
            auto instructions = pcode::Parser::Parse(text, regRepo);
            for (auto& instr : instructions) {
                if (instr.getInput0() != nullptr)
                    ExportObject(instr.getInput0());
                if (instr.getInput1() != nullptr)
                    ExportObject(instr.getInput1());
                if (instr.getOutput() != nullptr)
                    ExportObject(instr.getOutput());
            }
            return instructions;
        }
    public:
        static void Init(v8pp::module& module) {
            v8pp::class_<pcode::Parser> cl(module.isolate());
            cl
                .static_method("Parse", &Parse);
            module.class_("PcodeParser", cl);
        }
    };

    class PcodePrinterBind : public AbstractPrinterBind
    {
    public:
        static void Init(v8pp::module& module) {
            using PrinterJs = PrinterJs<pcode::Printer, const RegisterRepository*>;
            auto cl = PrinterJs::Create(module);
            cl
                .method("printInstruction", &PrinterJs::printInstruction)
                .method("printVarnode", &PrinterJs::printVarnode)
                .static_method("Print", &PrinterJs::Print);
            module.class_("PcodePrinter", cl);
        }
    };
};