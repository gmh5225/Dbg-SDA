#include "Disasm/FunctionCallLookup.h"
#include "Core/Utils.h"

using namespace sda;
using namespace sda::disasm;

const std::list<std::pair<pcode::InstructionOffset, pcode::InstructionOffset>>& FunctionCallLookupCallbacks::getConstFunctionOffsets() const {
    return m_constFunctionOffsets;
}

void FunctionCallLookupCallbacks::onInstructionPassed(const pcode::Instruction* instr, pcode::InstructionOffset nextOffset) {
    if (instr->getId() == pcode::InstructionId::CALL) {
        auto targetOffset = getTargetOffset(instr);
        if (targetOffset == InvalidOffset)
            return;
        m_constFunctionOffsets.push_back({ instr->getOffset(), targetOffset });
        m_builder->addUnvisitedOffset(targetOffset);
    }

    if (m_nextCallbacks)
        m_nextCallbacks->onInstructionPassed(instr, nextOffset);
}