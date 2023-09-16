#include "AudioSampleFactory.h"
#include <iostream>
#include "port/importer/types/AudioSample.h"
#include "spdlog/spdlog.h"

std::shared_ptr<LUS::IResource> CubeOS::AudioSampleFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<AudioSample>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<AudioSampleFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load AudioBank with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void CubeOS::AudioSampleFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<AudioSample> bank = std::static_pointer_cast<AudioSample>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, bank);

    bank->loop.start = reader->ReadUInt32();
    bank->loop.end = reader->ReadUInt32();
    bank->loop.count = reader->ReadInt32();
    bank->loop.pad = reader->ReadInt32();

    uint32_t stateSize = reader->ReadUInt32();
    std::vector<int16_t> state;
    if(stateSize > 0){
        bank->loop.state = new int16_t[stateSize];
        reader->Read((char*) bank->loop.state, stateSize * sizeof(int16_t));
    } else {
        bank->loop.state = nullptr;
    }

    bank->book.order = reader->ReadInt32();
    bank->book.npredictors = reader->ReadInt32();

    uint32_t tableSize = reader->ReadUInt32();
    bank->book.book = new int16_t[tableSize];
    reader->Read((char*) bank->book.book, tableSize * sizeof(int16_t));

    int32_t sampleSize = reader->ReadInt32();
    char* sampleData = new char[sampleSize];
    reader->Read(sampleData, sampleSize);

    bank->mData.unused = 0;
    bank->mData.loaded = 1;
    bank->mData.loop = &bank->loop;
    bank->mData.book = &bank->book;
    bank->mData.sampleAddr = (uint8_t*) sampleData;
    bank->mData.sampleSize = sampleSize;
}