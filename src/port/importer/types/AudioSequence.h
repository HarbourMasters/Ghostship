#pragma once

#include <cstdint>
#include <Resource.h>

struct AudioSequenceData {
    uint32_t bankCount;
    uint8_t* banks;
    uint8_t* data;
    uint8_t id;
};

namespace SM64 {

class AudioSequence : public Ship::Resource<AudioSequenceData> {
  public:
    using Resource::Resource;

    AudioSequence() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    AudioSequenceData* GetPointer();
    size_t GetPointerSize();

    AudioSequenceData mData;
    std::vector<uint8_t> banks;
    std::vector<uint8_t> sampleData;
};
}