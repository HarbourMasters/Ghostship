#include "Painting.h"

namespace SM64 {
PaintingData* Painting::GetPointer() {
    return &mData;
}

size_t Painting::GetPointerSize() {
    return sizeof(mData);
}
} // namespace SM64