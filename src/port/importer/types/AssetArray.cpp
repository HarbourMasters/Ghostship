#include "AssetArray.h"

namespace SM64 {
uintptr_t* AssetArray::GetPointer() {
    return mPtrs.data();
}

size_t AssetArray::GetPointerSize() {
    return mPtrs.size() * sizeof(uintptr_t);
}
} // namespace SM64