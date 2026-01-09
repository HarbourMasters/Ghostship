#include "Dictionary.h"

namespace SM64 {
GameDictionary* Dictionary::GetPointer() {
    return &mData;
}

size_t Dictionary::GetPointerSize() {
    return sizeof(mData);
}
} // namespace SM64