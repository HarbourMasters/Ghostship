#include "Trajectory.h"

namespace SM64 {
TrajectoryData* Trajectory::GetPointer() {
    return mData.data();
}

size_t Trajectory::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(TrajectoryData);
}
}