#include "Movtex.h"

namespace SM64 {
int16_t* Movtex::GetPointer() {
    return mData.data();
}

size_t Movtex::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(int16_t);
}
}