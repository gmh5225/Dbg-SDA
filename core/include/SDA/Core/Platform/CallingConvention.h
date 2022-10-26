#pragma once
#include <map>
#include <string>
#include "SDA/Core/Utils/Serialization.h"
#include "SDA/Core/Offset.h"

namespace sda
{
    class SignatureDataType;

    class CallingConvention
    {
    public:
        struct Storage {
            enum UseType {
                Read,
                Write
            };
            UseType useType;
            size_t registerId;
            Offset offset = 0;

            bool operator<(const Storage& other) const;
        };

        struct StorageInfo {
            enum Type {
                None,
                Return,
                Parameter
            };
            Type type = None;
            size_t paramIdx = 0;
            bool isStoringFloat = false;
        };
        
        using Map = std::map<Storage, StorageInfo>;

        virtual std::string getName() const = 0;

        virtual Map getStorages(const SignatureDataType* signatureDt) const = 0;

        virtual bool getStorageInfo(const Storage& storage, StorageInfo& storageInfo) const = 0;
    };

    class CustomCallingConvention : public CallingConvention, public utils::ISerializable
    {
        Map m_storages;
    public:
        static inline const std::string Name = "custom";

        CustomCallingConvention() = default;

        CustomCallingConvention(Map storages);

        std::string getName() const override;

        Map getStorages(const SignatureDataType* signatureDt) const override;

        bool getStorageInfo(const Storage& storage, StorageInfo& storageInfo) const override;

        void serialize(boost::json::object& data) const override;

        void deserialize(boost::json::object& data) override;
    };
};