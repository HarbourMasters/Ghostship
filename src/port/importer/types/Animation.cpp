#include "Animation.h"

namespace CubeOS {
AnimationData* Animation::GetPointer() {
    return &mData;
}

size_t Animation::GetPointerSize() {
    return sizeof(mData);
}
}