#include "Animation.h"

namespace SM64 {
AnimationData* Animation::GetPointer() {
    return &mData;
}

size_t Animation::GetPointerSize() {
    return sizeof(mData);
}
}