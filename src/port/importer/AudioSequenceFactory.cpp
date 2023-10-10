#include "AudioSequenceFactory.h"
#include "port/importer/types/AudioSequence.h"
#include "spdlog/spdlog.h"
#include "port/Engine.h"
#include "port/importer/types/AudioBank.h"

std::shared_ptr<LUS::IResource> CubeOS::AudioSequenceFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<AudioSequence>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<AudioSequenceFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load AudioBank with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void CubeOS::AudioSequenceFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<AudioSequence> bank = std::static_pointer_cast<AudioSequence>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, bank);

    uint8_t id = reader->ReadUInt32();
    size_t bankCount = reader->ReadUInt32();
    for(size_t i = 0; i < bankCount; i++){
        std::string bankName = reader->ReadString();
        bank->banks.push_back(GameEngine::GetBankIdByName(bankName));
    }

    size_t sampleSize = reader->ReadUInt32();
    for(size_t i = 0; i < sampleSize; i++){
        bank->sampleData.push_back(reader->ReadUByte());
    }

    bank->mData.bankCount = bankCount;
    bank->mData.banks = bank->banks.data();
    bank->mData.data = bank->sampleData.data();
    bank->mData.id = id;
}