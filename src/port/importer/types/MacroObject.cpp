#include "MacroObject.h"

namespace SM64 {
int16_t* MacroObject::GetPointer() {
    return mData.data();
}

size_t MacroObject::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(int16_t);
}
}