#pragma once

#include <Resource.h>

namespace SM64 {
    class AssetArray : public Ship::Resource<uintptr_t> {
    public:
        using Resource::Resource;

        AssetArray() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

        uintptr_t* GetPointer();
        size_t GetPointerSize();

        std::vector<uintptr_t> mPtrs;
    };
}