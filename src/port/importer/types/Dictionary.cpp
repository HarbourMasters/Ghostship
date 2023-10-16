#include "Dictionary.h"

namespace CubeOS {
GameDictionary* Dictionary::GetPointer() {
    return &mData;
}

size_t Dictionary::GetPointerSize() {
    return sizeof(mData);
}
}