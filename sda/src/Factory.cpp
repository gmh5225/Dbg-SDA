#include "Factory.h"
#include "Core/Context.h"
#include "Core/Image/AddressSpace.h"
#include "Core/Image/Image.h"
#include "Core/DataType/VoidDataType.h"
#include "Core/DataType/PointerDataType.h"
#include "Core/DataType/ArrayDataType.h"
#include "Core/DataType/TypedefDataType.h"
#include "Core/DataType/ScalarDataType.h"
#include "Core/DataType/EnumDataType.h"
#include "Core/DataType/SignatureDataType.h"
#include "Core/DataType/StructureDataType.h"
#include "Core/Symbol/VariableSymbol.h"
#include "Core/Symbol/FunctionSymbol.h"
#include "Core/Symbol/FunctionParameterSymbol.h"
#include "Core/Symbol/StructureFieldSymbol.h"
#include "Core/SymbolTable/StandartSymbolTable.h"
#include "Core/SymbolTable/OptimizedSymbolTable.h"

using namespace sda;

ISerializable* IFactory::create(boost::json::object& data) {
    // uuid
    if (data.find("uuid") == data.end())
        throw std::runtime_error("Object without uuid");
    std::string uuid(data["uuid"].get_string().c_str());
    auto id = boost::uuids::string_generator()(uuid);

    // collection name
    std::string collection(data["collection"].get_string().c_str());

    return create(&id, collection, data);
}

Factory::Factory(Context* context)
    : m_context(context)
{}

ISerializable* Factory::create(boost::uuids::uuid* id, const std::string& collection, boost::json::object& data) {
    if (collection == AddressSpace::Collection) {
        return new AddressSpace(m_context, id);
    } else if (collection == Image::Collection) {
        if(data.find("rw") == data.end() || data.find("analyser") == data.end())
            throw std::runtime_error("Image without rw or analyser");

        std::string rwType(data["rw"].get_object()["type"].get_string().c_str());
        std::unique_ptr<IImageRW> rw;
        if (rwType == FileImageRW::Name)
            rw = std::make_unique<FileImageRW>();

        if (rw) {
            std::string analyserType(data["analyser"].get_object()["type"].get_string().c_str());
            std::shared_ptr<ImageAnalyser> analyser;
            if (analyserType == PEImageAnalyser::Name)
                analyser = std::make_shared<PEImageAnalyser>();

            if (analyser) {
                return new Image(m_context, std::move(rw), analyser, id);
            }
        }
    } else if (collection == DataType::Collection) {
        if (data.find("type") != data.end()) {
            std::string type(data["type"].get_string().c_str());
            if (type == VoidDataType::Type)
                return new VoidDataType(m_context, id);
            else if (type == PointerDataType::Type)
                return new PointerDataType(m_context, id);
            else if (type == ArrayDataType::Type)
                return new ArrayDataType(m_context, id);
            else if (type == TypedefDataType::Type)
                return new TypedefDataType(m_context, id);
            else if (type == ScalarDataType::Type)
                return new ScalarDataType(m_context, id);
            else if (type == EnumDataType::Type)
                return new EnumDataType(m_context, id);
            else if (type == StructureDataType::Type)
                return new StructureDataType(m_context, id);
            else if (type == SignatureDataType::Type) {
                if(data.find("calling_convention") == data.end())
                    throw std::runtime_error("SignatureDataType without callingConvention");
                    
                std::string ccType(data["calling_convention"].get_object()["type"].get_string().c_str());
                std::shared_ptr<CallingConvention> callingConvention;
                if (ccType == CustomCallingConvention::Name)
                    callingConvention = std::make_shared<CustomCallingConvention>();
                else {
                    for (auto& cc : m_context->getPlatform()->getCallingConventions()) {
                        if (ccType == cc->getName()) {
                            callingConvention = cc;
                            break;
                        }
                    }
                }

                if (callingConvention) {
                    return new SignatureDataType(m_context, callingConvention, id);
                }
            }
        }
    } else if (collection == Symbol::Collection) {
        if (data.find("type") != data.end()) {
            std::string type(data["type"].get_string().c_str());
            if (type == VariableSymbol::Type)
                return new VariableSymbol(m_context, id);
            else if (type == FunctionSymbol::Type)
                return new FunctionSymbol(m_context, id);
            else if (type == FunctionParameterSymbol::Type)
                return new FunctionParameterSymbol(m_context, id);
            else if (type == StructureFieldSymbol::Type)
                return new StructureFieldSymbol(m_context, id);
        }
    } else if (collection == SymbolTable::Collection) {
        if (data.find("type") != data.end()) {
            std::string type(data["type"].get_string().c_str());
            if (type == StandartSymbolTable::Type)
                return new StandartSymbolTable(m_context, id);
            else if (type == OptimizedSymbolTable::Type)
                return new OptimizedSymbolTable(m_context, id);
        }
    }

    for (auto& factory : m_factories) {
        if (auto object = factory->create(id, collection, data))
            return object;
    }

    throw std::runtime_error("Object cannot be created");
}

void Factory::add(std::unique_ptr<IFactory> factory) {
    m_factories.push_back(std::move(factory));
}