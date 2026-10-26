#ifndef GAME_VALUES_H
#define GAME_VALUES_H

#include "types.h"
#include "port/events/Events.h"

extern_s s32 game_value(s32 id, s32 vanilla);
extern_s f32 game_float(s32 id, f32 vanilla);
extern_s const Trajectory *game_trajectory(s32 id, const Trajectory *vanilla);

#endif // GAME_VALUES_H
