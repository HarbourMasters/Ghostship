#include "Dialog.h"

namespace CubeOS {
DialogEntry* Dialog::GetPointer() {
    return &mData;
}

size_t Dialog::GetPointerSize() {
    return sizeof(mData);
}
}