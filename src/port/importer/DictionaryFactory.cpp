#include "DictionaryFactory.h"
#include "port/importer/types/Dictionary.h"
#include "spdlog/spdlog.h"


std::shared_ptr<Ship::IResource> SM64::DictionaryFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                                           std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<Dictionary> dictionary = std::make_shared<Dictionary>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

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

    return dictionary;
}