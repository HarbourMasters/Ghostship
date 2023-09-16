#include "AudioBankFactory.h"
#include "port/importer/types/AudioBank.h"
#include "spdlog/spdlog.h"
#include "resourcebridge.h"

std::shared_ptr<LUS::IResource> CubeOS::AudioBankFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<AudioBank>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<AudioBankFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load AudioBank with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}



void CubeOS::AudioBankFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<AudioBank> bank = std::static_pointer_cast<AudioBank>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, bank);

    uint32_t instrumentCount = reader->ReadUInt32();

    for(size_t i = 0; i < instrumentCount; i++){
        auto* instrument = new Instrument();
//        memset(instrument, 0, sizeof(Instrument));
        bool valid = reader->ReadUByte();
        if(!valid){
            bank->instruments.push_back(nullptr);
            continue;
        }
        instrument->loaded = 1;
        instrument->releaseRate = reader->ReadUByte();
        instrument->normalRangeLo = reader->ReadUByte();
        instrument->normalRangeHi = reader->ReadUByte();

        uint32_t envelopeSize = reader->ReadUInt32();
        if(envelopeSize != 0){
            instrument->envelope = new AdsrEnvelope[envelopeSize];
            for(size_t j = 0; j < envelopeSize; j++){
                instrument->envelope[j].delay = BSWAP16(reader->ReadInt16());
                instrument->envelope[j].arg = BSWAP16(reader->ReadInt16());
            }
        }

        uint32_t soundFlags = reader->ReadUInt32();
        bool hasLo = soundFlags & (1 << 0);
        bool hasMed = soundFlags & (1 << 1);
        bool hasHi = soundFlags & (1 << 2);

        if(hasLo){
            std::string lowSampleName = reader->ReadString();
            instrument->lowNotesSound.sample = static_cast<AudioBankSample *>(ResourceGetDataByName(lowSampleName.c_str()));
            instrument->lowNotesSound.tuning = reader->ReadFloat();
        }

        if(hasMed){
            std::string normalSampleName = reader->ReadString();
            instrument->normalNotesSound.sample = static_cast<AudioBankSample *>(ResourceGetDataByName(normalSampleName.c_str()));
            instrument->normalNotesSound.tuning = reader->ReadFloat();
        }

        if(hasHi){
            std::string highSampleName = reader->ReadString();
            instrument->highNotesSound.sample = static_cast<AudioBankSample *>(ResourceGetDataByName(highSampleName.c_str()));
            instrument->highNotesSound.tuning = reader->ReadFloat();
        }

        bank->instruments.push_back(instrument);
    }

    uint32_t drumCount = reader->ReadUInt32();

    for(size_t i = 0; i < drumCount; i++){
        auto* drum = new Drum();
        drum->releaseRate = reader->ReadUByte();
        drum->pan = reader->ReadUByte();
        drum->loaded = 1;

        uint32_t envelopeSize = reader->ReadUInt32();
        if(envelopeSize != 0){
            drum->envelope = new AdsrEnvelope[envelopeSize];
            for(size_t j = 0; j < envelopeSize; j++){
                drum->envelope[j].delay = BSWAP16(reader->ReadInt16());
                drum->envelope[j].arg = BSWAP16(reader->ReadInt16());
            }
        }

        std::string sampleName = reader->ReadString();
        drum->sound.sample = static_cast<AudioBankSample *>(ResourceGetDataByName(sampleName.c_str()));
        drum->sound.tuning = reader->ReadFloat();

        bank->drums.push_back(drum);
    }

    bank->mData.unused = 1;
    bank->mData.numInstruments = instrumentCount;
    bank->mData.numDrums = drumCount;
    bank->mData.instruments = bank->instruments.data();
    bank->mData.drums = bank->drums.data();
}