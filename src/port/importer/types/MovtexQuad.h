#pragma once

#include <cstdint>
#include <ship/resource/Resource.h>

namespace SM64 {

struct MovtexQuadData {
    int16_t id;
    int16_t* quadArraySegmented;
};

class MovtexQuad : public Ship::Resource<MovtexQuadData> {
  public:
    using Resource::Resource;

    MovtexQuad() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    MovtexQuadData* GetPointer();
    size_t GetPointerSize();

    std::vector<MovtexQuadData> mData;
};
}