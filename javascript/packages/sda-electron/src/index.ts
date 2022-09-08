import core from "sda-core";
import platform from "sda-platform-x86";

const platformX86 = platform.PlatformX86.New(true);
const context = core.Context.New(platformX86);
console.log("Platform: " + platformX86.name);
const fastcallCC = platformX86.callingConventions[0];
console.log("CC: " + fastcallCC.name);
console.log("CC: " + fastcallCC.name);
console.log("reg = " + platformX86.registerRepository.getRegisterName(90));

function test1() {
    const callbacks = core.ContextCallbacksImpl.New();
    callbacks.oldCallbacks = context.callbacks;
    callbacks.onObjectAdded = (obj) => {
        
        console.log("Object added (id = " + obj.id + ")");
        const data = obj.serialize();
        console.log("Serialized data:");
        console.log(data);

        if (obj instanceof core.ContextObject) {
            const ctxObj = obj as core.ContextObject;
            console.log("--- name: " + ctxObj.name);
        }
    };
    context.callbacks = callbacks;

    const dataType = core.VoidDataType.New(context);
    if (dataType.isVoid) {
        console.log("Void data type! (size: " + dataType.size + ")");
    }

    const ptrDt = dataType.getPointerTo();
    console.log("Pointer to void data type! (base: " + ptrDt.baseType.name + ")");

    //await new Promise(r => setTimeout(r, 5000));

    console.log('end');
}

function test2() {
    const enumDt = core.EnumDataType.New(context, "MyEnum", { 1: "One", 2: "Two" });
    console.log("Enum data type! (size: " + enumDt.size + ")");
    console.log("Fields:");
    for (const key in enumDt.fields) {
        console.log("--- " + enumDt.fields[key] + " = " + key);
    }
}

function test3() {
    const dataType = core.ScalarDataType.New(context, "MyScalar", "FloatingPoint", 4);
    const symbolTable = core.StandartSymbolTable.New(context, "MySymbolTable");
    symbolTable.addSymbol(0x1000, core.VariableSymbol.New(context, "var1", dataType));
    symbolTable.addSymbol(0x1004, core.VariableSymbol.New(context, "var2", dataType));

    const symbolInfo = symbolTable.getSymbolAt(0x1000);
    console.log("Symbol info:");
    console.log("--- name: " + symbolInfo.symbol.name);
    console.log("--- offset: " + symbolInfo.symbolOffset);
    console.log("--- table: " + symbolInfo.symbolTable.name);
}

test3();