#pragma once

#include <cstdint>
#include <Resource.h>

struct AudioSequenceData {
    uint32_t bankCount;
    uint8_t* banks;
    uint8_t* data;
};

namespace CubeOS {

class AudioSequence : public LUS::Resource<AudioSequenceData> {
  public:
    using Resource::Resource;

    AudioSequence() : Resource(std::shared_ptr<LUS::ResourceInitData>()) {}

    AudioSequenceData* GetPointer();
    size_t GetPointerSize();

    AudioSequenceData mData;
    std::vector<uint8_t> banks;
    std::vector<uint8_t> sampleData;
};
}