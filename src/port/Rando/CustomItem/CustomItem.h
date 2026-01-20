#include "port/Rando/StaticData/StaticData.h"

extern "C" {
#include "sm64.h"
#include "include/types.h"
#include "game/area.h"
#include "game/save_file.h"
}

namespace CustomItem {
extern int16_t redCoinsCollected;

void ObjectCollected(int16_t type, struct MarioState* mario, struct Object* object);
void SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId, RandoAct randoAct);
void SpawnObject(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z, s32 param, RandoCheckId randoCheckId,
                 RandoAct randoAct);
} // namespace CustomItem
