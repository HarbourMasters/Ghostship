#include "AudioSequence.h"

namespace CubeOS {
AudioSequenceData* AudioSequence::GetPointer() {
    return &mData;
}

size_t AudioSequence::GetPointerSize() {
    return sizeof(mData);
}
}