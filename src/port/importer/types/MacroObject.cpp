#include "MacroObject.h"

namespace SM64 {
int16_t* MacroObjectResource::GetPointer() {
    return mData.data();
}

size_t MacroObjectResource::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(int16_t);
}
} // namespace SM64