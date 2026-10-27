#include <libultraship.h>

#include "game_values.h"

s32 game_value(s32 id, s32 vanilla) {
    s32 value = vanilla;
    CALL_EVENT(GameValue, id, &value);
    return value;
}

f32 game_float(s32 id, f32 vanilla) {
    f32 value = vanilla;
    CALL_EVENT(GameFloat, id, &value);
    return value;
}

const Trajectory *game_trajectory(s32 id, const Trajectory *vanilla) {
    const Trajectory *path = vanilla;
    CALL_EVENT(ModifyTrajectory, id, &path);
    return path;
}
