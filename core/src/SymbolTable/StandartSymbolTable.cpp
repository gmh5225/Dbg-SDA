#include "Core/SymbolTable/StandartSymbolTable.h"
#include "Core/Symbol/Symbol.h"
#include "Core/DataType/DataType.h"

using namespace sda;

StandartSymbolTable::StandartSymbolTable(Context* context, Object::Id* id, const std::string& name)
    : SymbolTable(context, id, name)
{}

void StandartSymbolTable::addSymbol(Offset offset, Symbol* symbol) {
    auto [_, _, symbol] = getSymbolAt(offset);
    assert(!symbol);
    m_context->getCallbacks()->onObjectModified(this);
    m_symbols[offset] = symbol;
}

void StandartSymbolTable::removeSymbol(Offset offset) {
    m_context->getCallbacks()->onObjectModified(this);
    auto [_, symbolOffset, symbol] = getSymbolAt(offset);
    if (symbol) {
        m_symbols.erase(symbolOffset);
    }
}

SymbolTable::SymbolInfo StandartSymbolTable::getSymbolAt(Offset offset) {
    if (!m_symbols.empty()) {
        const auto it = std::prev(m_symbols.upper_bound(offset));
        if (it != m_symbols.end()) {
            const auto& [symbolOffset, symbol] = *it;
            if (offset < symbolOffset + symbol->getDataType()->getSize()) {
                return {
                    this,
                    symbolOffset,
                    symbol
                };
            }
        }
    }
    return {
        this,
        0,
        nullptr
    };
}

const std::map<Offset, Symbol*>& StandartSymbolTable::getSymbols() const {
    return m_symbols;
}

void StandartSymbolTable::setSymbols(const std::map<Offset, Symbol*>& symbols) {
    m_context->getCallbacks()->onObjectModified(this);
    m_symbols = symbols;
}

void StandartSymbolTable::serialize(boost::json::object& data) const {
    SymbolTable::serialize(data);
    data["type"] = Type;

    // serialize all symbols
    boost::json::array symbols;
    for (const auto& [offset, symbol] : m_symbols) {
        boost::json::object symbolData;
        symbolData["offset"] = offset;
        symbolData["symbol"] = symbol->serializeId();
    }
    data["symbols"] = symbols;
}

void StandartSymbolTable::deserialize(boost::json::object& data) {
    SymbolTable::deserialize(data);

    // deserialize all symbols
    m_symbols.clear();
    const auto& symbols = data["symbol"].get_array();
    for (auto symbolData : symbols) {
        auto offset = symbolData.get_object()["offset"].get_uint64();
        auto symbol = m_context->getSymbols()->get(symbolData.get_object()["symbol"]);
        m_symbols[offset] = symbol;
    }
}