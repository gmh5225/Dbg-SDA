#include "Database/Transaction.h"
#include "Database/Database.h"

using namespace sda;

Transaction::Transaction(Database* database)
    : m_database(database)
{}

void Transaction::markAsNew(utils::ISerializable* obj) {
    m_items.push_back({ Item::New, obj });
}

void Transaction::markAsModified(utils::ISerializable* obj) {
    m_items.push_back({ Item::Modified, obj });
}

void Transaction::markAsRemoved(utils::ISerializable* obj) {
    m_items.push_back({ Item::Removed, obj });
}

void Transaction::commit() {
    for(auto item : m_items) {
        boost::json::object data;
        item.object->serialize(data);
        if (data["temporary"].get_bool())
            continue;
        auto collectionName = std::string(data["collection"].get_string().c_str());
        auto collection = m_database->getCollection(collectionName);
        if (item.type == Item::New || item.type == Item::Modified) {
            collection->write(data);
        } else if (item.type == Item::Removed) {
            collection->remove(data);
        }
    }

    m_items.clear();
}