#include "DictionaryFactory.h"
#include "port/importer/types/Dictionary.h"
#include "spdlog/spdlog.h"

std::shared_ptr<LUS::IResource> CubeOS::DictionaryFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<Dictionary>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<DictionaryFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Dialog with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void CubeOS::DictionaryFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<Dictionary> dictionary = std::static_pointer_cast<Dictionary>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, dictionary);

    size_t numEntries = reader->ReadUInt32();
    for(size_t i = 0; i < numEntries; i++){
        std::string key = reader->ReadString();
        size_t valueSize = reader->ReadUInt32();
        std::vector<uint8_t> value;
        for(size_t j = 0; j < valueSize; j++){
            value.push_back(reader->ReadUByte());
        }
        dictionary->mData[key] = value;
    }
}