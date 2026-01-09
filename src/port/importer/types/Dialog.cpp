#include "Dialog.h"

namespace SM64 {
DialogEntry* Dialog::GetPointer() {
    return &mData;
}

size_t Dialog::GetPointerSize() {
    return sizeof(mData);
}
}