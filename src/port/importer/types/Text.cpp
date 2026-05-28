#include "Text.h"

namespace SM64 {
Text::Text() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

void* Text::GetPointer() {
    return &Data;
}

size_t Text::GetPointerSize() {
    return Data.size();
}
} // namespace SM64