#pragma once

#include <cstdint>

#include <ship/resource/Resource.h>
#include <libultraship/libultra/types.h>

namespace SM64 {
struct AnimationData {
    /*0x00*/ int16_t flags;
    /*0x02*/ int16_t animYTransDivisor;
    /*0x04*/ int16_t startFrame;
    /*0x06*/ int16_t loopStart;
    /*0x08*/ int16_t loopEnd;
    /*0x0A*/ int16_t unusedBoneCount;
    /*0x0C*/ const int16_t *values;
    /*0x10*/ const uint16_t *index;
    /*0x14*/ uint64_t length; // only used with Mario animations to determine how much to load. 0 otherwise.
};

class Animation : public Ship::Resource<AnimationData> {
  public:
    using Resource::Resource;

    Animation() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    AnimationData* GetPointer();
    size_t GetPointerSize();

    AnimationData mData;

    std::vector<uint16_t> indices;
    std::vector<int16_t> values;
};
}