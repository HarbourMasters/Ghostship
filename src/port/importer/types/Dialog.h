#pragma once

#include <cstdint>
#include <Resource.h>

struct DialogEntry {
    /*0x00*/ uint32_t unused;
    /*0x04*/ int8_t linesPerBox;
    /*0x06*/ int16_t leftOffset;
    /*0x08*/ int16_t width;
    /*0x0C*/ uint8_t *str;
};


namespace CubeOS {

class Dialog : public LUS::Resource<DialogEntry> {
  public:
    using Resource::Resource;

    Dialog() : Resource(std::shared_ptr<LUS::ResourceInitData>()) {}

    DialogEntry* GetPointer();
    size_t GetPointerSize();

    DialogEntry mData;
    std::vector<uint8_t> mText;
};
}