#pragma once

#include <cstdint>
#include <ship/resource/Resource.h>

namespace SM64 {

class Movtex : public Ship::Resource<int16_t> {
  public:
    using Resource::Resource;

    Movtex() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    int16_t* GetPointer();
    size_t GetPointerSize();

    std::vector<int16_t> mData;
};
}