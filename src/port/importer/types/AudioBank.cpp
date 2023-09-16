#include "AudioBank.h"

namespace CubeOS {
CtlEntry* AudioBank::GetPointer() {
    return &mData;
}

size_t AudioBank::GetPointerSize() {
    return sizeof(mData);
}
}