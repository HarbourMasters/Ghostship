#include "include/types.h"
#include "game/area.h"

extern "C" {
#include "sm64.h"
}

namespace CustomItem {

void SpawnObject(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z, s32 param);
}; // namespace CustomItem
