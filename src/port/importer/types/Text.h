#pragma once

#include <ship/resource/Resource.h>
#include <libultraship/libultra/types.h>

namespace SM64 {

class Text : public Ship::Resource<void> {
  public:
    using Resource::Resource;

    Text();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::string Data;
};
}; // namespace SM64 