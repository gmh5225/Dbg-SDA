#include "Decompiler/Semantics/SemanticsPropagator.h"
#include "Decompiler/Semantics/SemanticsManager.h"
#include "Core/DataType/ScalarDataType.h"
#include "Core/DataType/PointerDataType.h"
#include "Core/DataType/ArrayDataType.h"
#include "Core/DataType/StructureDataType.h"

using namespace sda;
using namespace sda::decompiler;

SemanticsPropagator::SemanticsPropagator(SemanticsManager* semManager)
    : m_semManager(semManager)
{}

SemanticsManager* SemanticsPropagator::getManager() const {
    return m_semManager;
}

VariableSemObj* SemanticsPropagator::getOrCreateVarObject(std::shared_ptr<ircode::Variable> var) const {
    auto id = VariableSemObj::GetId(var.get());
    if (auto obj = getManager()->getObject<VariableSemObj>(id))
        return obj;
    auto newObj = getManager()->addObject(std::make_unique<VariableSemObj>(var.get()));
    return dynamic_cast<VariableSemObj*>(newObj);
}

void SemanticsPropagator::bindEachOther(SemanticsObject* obj1, SemanticsObject* obj2) const {
    obj1->bindTo(obj2);
    obj2->bindTo(obj1);
}

void SemanticsPropagator::propagateTo(
    SemanticsObject* fromObj,
    SemanticsObject* toObj,
    Semantics::FilterFunction filter,
    std::set<SemanticsObject*>& nextObjs,
    size_t uncertaintyDegree) const
{
    if (getManager()->isSimiliarityConsidered()) {
        auto toObjSemantics = toObj->findSemantics(filter);
        auto simFilter = [toObjSemantics](const Semantics* sem) {
            for (auto sem2 : toObjSemantics) {
                if (sem == sem2 || sem->isSimiliarTo(sem2))
                    return false;
            }
            return true;
        };
        filter = Semantics::FilterAnd(filter, simFilter);
    }

    bool isPropagated = false;
    auto fromObjSemantics = fromObj->findSemantics(filter);
    for (auto sem : fromObjSemantics) {
        if (uncertaintyDegree > 0) {
            auto newUncertaintyDegree = sem->getUncertaintyDegree() + uncertaintyDegree;
            auto newSem = getManager()->addSemantics(sem->clone(newUncertaintyDegree));
            sem->addSuccessor(newSem);
            sem = newSem;
        }
        isPropagated |= toObj->addSemantics(sem);
    }

    if (isPropagated)
        nextObjs.insert(toObj);
}

void BaseSemanticsPropagator::propagate(
    const SemanticsContext* ctx,
    const ircode::Operation* op,
    std::set<SemanticsObject*>& nextObjs)
{
    auto output = op->getOutput();

    if (auto unaryOp = dynamic_cast<const ircode::UnaryOperation*>(op)) {
        auto input = unaryOp->getInput();
        if (unaryOp->getId() == ircode::OperationId::LOAD) {
            if (auto inputReg = std::dynamic_pointer_cast<ircode::Register>(input)) {
                auto signatureDt = ctx->functionSymbol->getSignature();
                auto it = signatureDt->getStorages().find({
                    CallingConvention::Storage::Read,
                    inputReg->getRegister().getRegId()
                });
                if (it != signatureDt->getStorages().end()) {
                    // if it is a parameter
                    const auto& storageInfo = it->second;
                    auto paramIdx = storageInfo.paramIdx;
                    auto paramSymbol = signatureDt->getParameters()[paramIdx];
                    if (auto paramSymbolObj = getSymbolObject(paramSymbol)) {
                        auto outputVarObj = getOrCreateVarObject(output);
                        bindEachOther(paramSymbolObj, outputVarObj);
                        propagateTo(paramSymbolObj, outputVarObj, DataTypeSemantics::Filter(), nextObjs);
                        propagateTo(outputVarObj, paramSymbolObj, DataTypeSemantics::Filter(), nextObjs);
                    }
                } else {
                    // if it is a stack or global variable
                    auto symbolTable = ctx->functionSymbol->getStackSymbolTable();
                    if (inputReg->getRegister().getRegId() == Register::InstructionPointerId)
                        symbolTable = ctx->globalSymbolTable;
                    if (symbolTable) {
                        if (auto symbolTableObj = getSymbolTableObject(symbolTable)) {
                            auto outputVarObj = getOrCreateVarObject(output);
                            bindEachOther(symbolTableObj, outputVarObj);
                            propagateTo(symbolTableObj, outputVarObj, SymbolTableSemantics::Filter(), nextObjs);
                        }
                    }
                }
            }
            else if (auto inputVar = std::dynamic_pointer_cast<ircode::Variable>(input)) {
                auto inputVarObj = getOrCreateVarObject(inputVar);
                auto outputVarObj = getOrCreateVarObject(output);
                auto loadSize = output->getSize();

                auto linearExpr = inputVar->getLinearExpr();
                Offset offset = linearExpr.getConstTermValue();
                auto symbolTables = getAllSymbolTables(linearExpr);
                for (auto& [sem, symbolTable] : symbolTables) {
                    const auto symbols = getAllSymbolsAt(ctx, symbolTable, offset);
                    for (auto& [symbolOffset, symbol] : symbols) {
                        auto symbolDt = symbol->getDataType();
                        if (loadSize <= symbolDt->getSize()) {
                            if (auto symbolObj = getSymbolObject(symbol)) {
                                bindEachOther(symbolObj, outputVarObj);
                                DataTypeSemantics::SliceInfo sliceInfo = {};
                                auto relOffset = offset - symbolOffset;
                                if (relOffset != 0 || loadSize != symbolDt->getSize()) {
                                    sliceInfo.type = DataTypeSemantics::SliceInfo::Load;
                                    sliceInfo.offset = relOffset;
                                    sliceInfo.size = loadSize;
                                }
                                auto newSem = createDataTypeSemantics(outputVarObj, symbolDt, sliceInfo); // outputVarObj or symbolObj?
                                sem->addSuccessor(newSem);
                            }
                        }
                    }
                }

                // if loading value at address that points to a scalar or array of scalars (not structures)
                if (symbolTables.empty()) {
                    auto semantics = inputVarObj->findSemantics(DataTypeSemantics::Filter());
                    for (auto sem : semantics) {
                        if (auto dataTypeSem = dynamic_cast<DataTypeSemantics*>(sem)) {
                            DataType* itemDt = nullptr;
                            if (auto pointerDt = dynamic_cast<PointerDataType*>(dataTypeSem->getDataType())) {
                                itemDt = pointerDt->getPointedType();
                            } else if (auto arrayDt = dynamic_cast<ArrayDataType*>(dataTypeSem->getDataType())) {
                                // "array" is just a pointer to repeated data elements but with max size constraint
                                itemDt = arrayDt->getElementType();
                                if (offset >= arrayDt->getSize())
                                    continue;
                            }
                            // structure data type should be handled above
                            assert(!dynamic_cast<StructureDataType*>(itemDt));
                            DataTypeSemantics::SliceInfo sliceInfo = {};
                            auto relOffset = offset % itemDt->getSize();
                            if (relOffset != 0 || loadSize != itemDt->getSize()) {
                                sliceInfo.type = DataTypeSemantics::SliceInfo::Load;
                                sliceInfo.offset = relOffset;
                                sliceInfo.size = loadSize;
                            }
                            auto newSem = createDataTypeSemantics(outputVarObj, itemDt, sliceInfo);
                            sem->addSuccessor(newSem);
                        }
                    }
                }
            }
        }
        else if (unaryOp->getId() == ircode::OperationId::COPY) {
            if (auto inputVar = std::dynamic_pointer_cast<ircode::Variable>(input)) {
                auto inputVarObj = getOrCreateVarObject(inputVar);
                auto outputVarObj = getOrCreateVarObject(output);
                propagateTo(inputVarObj, outputVarObj, Semantics::FilterAll(), nextObjs);
                propagateTo(outputVarObj, inputVarObj, Semantics::FilterAll(), nextObjs);
            }
        }
        else if (unaryOp->getId() == ircode::OperationId::INT_2COMP) {
            auto signedScalarDt = getScalarDataType(ScalarType::SignedInt, output->getSize());
            setDataTypeFor(input, signedScalarDt, nextObjs);
            setDataTypeFor(output, signedScalarDt, nextObjs);
        }
        else if (unaryOp->getId() == ircode::OperationId::BOOL_NEGATE) {
            auto booleanDt = findDataType("bool");
            setDataTypeFor(input, booleanDt, nextObjs);
            setDataTypeFor(output, booleanDt, nextObjs);
        }
        else if (unaryOp->getId() == ircode::OperationId::FLOAT_NEG ||
                 unaryOp->getId() == ircode::OperationId::FLOAT_ABS ||
                 unaryOp->getId() == ircode::OperationId::FLOAT_SQRT) {
            auto floatScalarDt = getScalarDataType(ScalarType::FloatingPoint, output->getSize());
            setDataTypeFor(input, floatScalarDt, nextObjs);
            setDataTypeFor(output, floatScalarDt, nextObjs);
        }
    }
    else if (auto binaryOp = dynamic_cast<const ircode::BinaryOperation*>(op)) {
        auto input1 = binaryOp->getInput1();
        auto input2 = binaryOp->getInput2();

        if (binaryOp->getId() == ircode::OperationId::INT_ADD ||
            binaryOp->getId() == ircode::OperationId::INT_SUB ||
            binaryOp->getId() == ircode::OperationId::INT_MULT ||
            binaryOp->getId() == ircode::OperationId::INT_DIV ||
            binaryOp->getId() == ircode::OperationId::INT_REM)
        {
            auto outputVarObj = getOrCreateVarObject(output);
            for (auto& input : {input1, input2}) {
                if (auto inputVar = std::dynamic_pointer_cast<ircode::Variable>(input)) {
                    auto inputVarObj = getOrCreateVarObject(inputVar);
                    auto signedScalarDt = getScalarDataType(ScalarType::SignedInt, output->getSize());
                    auto filter = DataTypeSemantics::Filter(signedScalarDt);
                    propagateTo(inputVarObj, outputVarObj, filter, nextObjs);
                    propagateTo(outputVarObj, inputVarObj, filter, nextObjs, 1);
                }
            }

            if (binaryOp->getId() == ircode::OperationId::INT_ADD ||
                binaryOp->getId() == ircode::OperationId::INT_MULT)
            {
                auto linearExpr = output->getLinearExpr();
                Offset offset = linearExpr.getConstTermValue();
                auto symbolTables = getAllSymbolTables(linearExpr);
                auto createVoidPtrDtSem = !symbolTables.empty();
                for (auto& [sem, symbolTable] : symbolTables) {
                    const auto symbols = getAllSymbolsAt(ctx, symbolTable, offset);
                    for (auto& [symbolOffset, symbol] : symbols) {
                        if (symbolOffset != offset)
                            continue;
                        if (auto symbolObj = getSymbolObject(symbol)) {
                            bindEachOther(symbolObj, outputVarObj);
                            auto symbolPtrDt = symbol->getDataType()->getPointerTo();
                            auto newSem = createDataTypeSemantics(outputVarObj, symbolPtrDt);
                            sem->addSuccessor(newSem);
                            createVoidPtrDtSem = false;
                        }
                    }
                }

                if (createVoidPtrDtSem) {
                    auto voidDt = findDataType("void");
                    auto newSem = createDataTypeSemantics(outputVarObj, voidDt->getPointerTo());
                    for (auto& [sem, _] : symbolTables) {
                        sem->addSuccessor(newSem);
                    }
                }
            }
        }
        else if (binaryOp->getId() == ircode::OperationId::INT_SDIV ||
                binaryOp->getId() == ircode::OperationId::INT_SREM)
        {
            auto signedScalarDt = getScalarDataType(ScalarType::SignedInt, output->getSize());
            setDataTypeFor(output, signedScalarDt, nextObjs);
        }
        else if (binaryOp->getId() >= ircode::OperationId::INT_EQUAL &&
                binaryOp->getId() <= ircode::OperationId::INT_LESSEQUAL ||
                binaryOp->getId() >= ircode::OperationId::FLOAT_EQUAL &&
                binaryOp->getId() <= ircode::OperationId::FLOAT_LESSEQUAL)
        {
            auto booleanDt = findDataType("bool");
            setDataTypeFor(output, booleanDt, nextObjs);
            if (binaryOp->getId() >= ircode::OperationId::FLOAT_EQUAL &&
                binaryOp->getId() <= ircode::OperationId::FLOAT_LESSEQUAL)
                {
                    auto floatScalarDt = getScalarDataType(ScalarType::FloatingPoint, output->getSize());
                    setDataTypeFor(input1, floatScalarDt, nextObjs);
                    setDataTypeFor(input2, floatScalarDt, nextObjs);
                }
        }
        else if (binaryOp->getId() >= ircode::OperationId::BOOL_NEGATE &&
                binaryOp->getId() <= ircode::OperationId::BOOL_OR)
        {
            auto booleanDt = findDataType("bool");
            setDataTypeFor(input1, booleanDt, nextObjs);
            setDataTypeFor(input2, booleanDt, nextObjs);
            setDataTypeFor(output, booleanDt, nextObjs);
        }
        else if (binaryOp->getId() >= ircode::OperationId::FLOAT_ADD &&
                binaryOp->getId() <= ircode::OperationId::FLOAT_SQRT)
        {
            auto floatScalarDt = getScalarDataType(ScalarType::FloatingPoint, output->getSize());
            setDataTypeFor(input1, floatScalarDt, nextObjs);
            setDataTypeFor(input2, floatScalarDt, nextObjs);
            setDataTypeFor(output, floatScalarDt, nextObjs);
        }
    }

    auto outputAddrVal = output->getMemAddress().value;
    if (auto outputAddrReg = std::dynamic_pointer_cast<ircode::Register>(outputAddrVal)) {
        auto signatureDt = ctx->functionSymbol->getSignature();
        auto it = signatureDt->getStorages().find({
            CallingConvention::Storage::Write,
            outputAddrReg->getRegister().getRegId()
        });
        if (it != signatureDt->getStorages().end()) {
            if (auto funcReturnObj = getFuncReturnObject(signatureDt)) {
                auto outputVarObj = getOrCreateVarObject(output);
                bindEachOther(funcReturnObj, outputVarObj);
                propagateTo(funcReturnObj, outputVarObj, DataTypeSemantics::Filter(), nextObjs);
                propagateTo(outputVarObj, funcReturnObj, DataTypeSemantics::Filter(), nextObjs);
            }
        }
    }
    else if (auto outputAddrVar = std::dynamic_pointer_cast<ircode::Variable>(outputAddrVal)) {
        auto outputVarObj = getOrCreateVarObject(output);
        auto linearExpr = outputAddrVar->getLinearExpr();
        Offset offset = linearExpr.getConstTermValue();
        auto symbolTables = getAllSymbolTables(linearExpr);
        for (auto& [sem, symbolTable] : symbolTables) {
            const auto symbols = getAllSymbolsAt(ctx, symbolTable, offset, true);
            for (auto& [symbolOffset, symbol] : symbols) {
                if (symbolOffset != offset)
                    continue;
                if (auto symbolObj = getSymbolObject(symbol)) {
                    bindEachOther(symbolObj, outputVarObj);
                    propagateTo(symbolObj, outputVarObj, DataTypeSemantics::Filter(), nextObjs);
                    propagateTo(outputVarObj, symbolObj, DataTypeSemantics::Filter(), nextObjs);
                }
            }
        }
    }
}

SymbolSemObj* BaseSemanticsPropagator::getSymbolObject(const Symbol* symbol) const {
    auto id = SymbolSemObj::GetId(symbol);
    if (auto obj = getManager()->getObject<SymbolSemObj>(id))
        return obj;
    return nullptr;
}

SymbolTableSemObj* BaseSemanticsPropagator::getSymbolTableObject(const SymbolTable* symbolTable) const {
    auto id = SymbolTableSemObj::GetId(symbolTable);
    if (auto obj = getManager()->getObject<SymbolTableSemObj>(id))
        return obj;
    return nullptr;
}

FuncReturnSemObj* BaseSemanticsPropagator::getFuncReturnObject(const SignatureDataType* signatureDt) const {
    auto id = FuncReturnSemObj::GetId(signatureDt);
    if (auto obj = getManager()->getObject<FuncReturnSemObj>(id))
        return obj;
    return nullptr;
}

DataType* BaseSemanticsPropagator::findDataType(const std::string& name) const {
    auto dataType = getManager()->getContext()->getDataTypes()->getByName(name);
    assert(dataType);
    return dataType;
}

ScalarDataType* BaseSemanticsPropagator::getScalarDataType(ScalarType scalarType, size_t size) const {
    return getManager()->getContext()->getDataTypes()->getScalar(scalarType, size);
}

DataTypeSemantics* BaseSemanticsPropagator::createDataTypeSemantics(
    SemanticsObject* sourceObject,
    const DataType* dataType,
    const DataTypeSemantics::SliceInfo& sliceInfo,
    size_t uncertaintyDegree) const
{
    auto sem = getManager()->addSemantics(std::make_unique<DataTypeSemantics>(sourceObject, dataType, sliceInfo, uncertaintyDegree));
    return dynamic_cast<DataTypeSemantics*>(sem);
}

void BaseSemanticsPropagator::setDataTypeFor(std::shared_ptr<ircode::Value> value, const DataType* dataType, std::set<SemanticsObject*>& nextObjs) const {
    if (auto var = std::dynamic_pointer_cast<ircode::Variable>(value)) {
        auto varObj = getOrCreateVarObject(var);
        bool onlyEmitted = !getManager()->isSimiliarityConsidered();
        if (!varObj->checkSemantics(DataTypeSemantics::Filter(dataType), onlyEmitted)) {
            auto sem = createDataTypeSemantics(varObj, dataType);
            nextObjs.insert(varObj);
        }
    }
}

std::list<std::pair<Semantics*, SymbolTable*>> BaseSemanticsPropagator::getAllSymbolTables(const ircode::LinearExpression& linearExpr) const {
    std::list<std::pair<Semantics*, SymbolTable*>> result;
    for (auto& term : linearExpr.getTerms()) {
        if (!term.canBePointer())
            continue;
        if (auto termVar = std::dynamic_pointer_cast<ircode::Variable>(term.value)) {
            auto termVarObj = getOrCreateVarObject(termVar);
            auto filter = Semantics::FilterOr(DataTypeSemantics::Filter(), SymbolTableSemantics::Filter());
            auto semantics = termVarObj->findSemantics(filter);
            for (auto sem : semantics) {
                if (auto symbolTableSem = dynamic_cast<SymbolTableSemantics*>(sem)) {
                    result.emplace_back(sem, symbolTableSem->getSymbolTable());
                } else if (auto dataTypeSem = dynamic_cast<DataTypeSemantics*>(sem)) {
                    if (auto pointerDt = dynamic_cast<const PointerDataType*>(dataTypeSem->getDataType()))
                        if (auto structDt = dynamic_cast<const StructureDataType*>(pointerDt->getPointedType()))
                            result.emplace_back(sem, structDt->getSymbolTable());
                }
            }
        }
    }
    return result;
}

std::list<std::pair<Offset, Symbol*>> BaseSemanticsPropagator::getAllSymbolsAt(const SemanticsContext* ctx, SymbolTable* symbolTable, Offset offset, bool write) const {
    std::list<std::pair<Offset, Symbol*>> symbols;
    if (symbolTable == ctx->globalSymbolTable ||
        symbolTable == ctx->functionSymbol->getStackSymbolTable())
    {
        CallingConvention::Storage storage;
        storage.useType = write ? CallingConvention::Storage::Write : CallingConvention::Storage::Read;
        storage.registerId = Register::StackPointerId;
        if (symbolTable == ctx->globalSymbolTable)
            storage.registerId = Register::InstructionPointerId;
        storage.offset = offset;
        auto signatureDt = ctx->functionSymbol->getSignature();
        auto it = signatureDt->getStorages().find(storage);
        if (it != signatureDt->getStorages().end()) {
            const auto& storageInfo = it->second;
            auto paramIdx = storageInfo.paramIdx;
            auto paramSymbol = signatureDt->getParameters()[paramIdx];
            symbols.emplace_back(offset, paramSymbol);
        }
    }
    if (symbols.empty()) {
        auto foundSymbols = symbolTable->getAllSymbolsRecursivelyAt(offset);
        for (auto [_, symbolOffset, symbol] : foundSymbols)
            symbols.emplace_back(symbolOffset, symbol);
    }
    return symbols;
}