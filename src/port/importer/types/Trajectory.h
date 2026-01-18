#pragma once

#include <cstdint>
#include <ship/resource/Resource.h>

struct TrajectoryData {
    int16_t trajId;
    int16_t posX;
    int16_t posY;
    int16_t posZ;
};

namespace SM64 {

class TrajectoryResource : public Ship::Resource<TrajectoryData> {
  public:
    using Resource::Resource;

    TrajectoryResource() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    TrajectoryData* GetPointer();
    size_t GetPointerSize();

    std::vector<TrajectoryData> mData;
};
}