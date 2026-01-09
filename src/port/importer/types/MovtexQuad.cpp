#include "MovtexQuad.h"

namespace SM64 {
MovtexQuadData* MovtexQuad::GetPointer() {
    return mData.data();
}

size_t MovtexQuad::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(MovtexQuadData);
}
} // namespace SM64