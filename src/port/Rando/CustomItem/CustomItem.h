#include "port/Rando/StaticData/StaticData.h"
#include "include/types.h"
#include "game/area.h"

extern "C" {
#include "sm64.h"
}

namespace CustomItem {
void SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId);
void SpawnObject(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z, s32 param,
                 RandoCheckId randoCheckId);
}; // namespace CustomItem
