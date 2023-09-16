#include "AudioSample.h"

namespace CubeOS {
AudioBankSample* AudioSample::GetPointer() {
    return &mData;
}

size_t AudioSample::GetPointerSize() {
    return sizeof(mData);
}
}