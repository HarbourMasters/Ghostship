#pragma once

#include <cstdint>
#include <unordered_map>
#include <Resource.h>

namespace SM64 {

typedef std::unordered_map<std::string, std::vector<uint8_t>> GameDictionary;

class Dictionary : public LUS::Resource<GameDictionary> {
  public:
    using Resource::Resource;

    Dictionary() : Resource(std::shared_ptr<LUS::ResourceInitData>()) {}

    GameDictionary* GetPointer();
    size_t GetPointerSize();

    GameDictionary mData;
};
}