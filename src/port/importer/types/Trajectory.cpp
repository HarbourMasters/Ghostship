#include "Trajectory.h"

namespace SM64 {
TrajectoryData* TrajectoryResource::GetPointer() {
    return mData.data();
}

size_t TrajectoryResource::GetPointerSize() {
    return sizeof(mData.size()) * sizeof(TrajectoryData);
}
} // namespace SM64