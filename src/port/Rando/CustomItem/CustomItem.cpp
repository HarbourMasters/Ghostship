#include "CustomItem.h"

extern "C" {
#include "game/object_list_processor.h"
#include "include/behavior_data.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
}

std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void CustomItem::SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId) {
    if (randoCheckId >= RC_WF_BLUE_COIN_01 && randoCheckId <= RC_WF_BLUE_COIN_04) {
        object->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
        object->oIntangibleTimer = -1;
    } else {
        switch (modelId) {
            case MODEL_BLUE_COIN:
                object->oAction = HIDDEN_BLUE_COIN_ACT_ACTIVE;
                break;
            case MODEL_RED_COIN:
                break;
            case MODEL_STAR:
                break;
            default:
                break;
        }
    }
}

void CustomItem::SpawnObject(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z, s32 param,
                             RandoCheckId randoCheckId) {
    struct Object* object =
        spawn_object_abs_with_rot(&gMacroObjectDefaultParent, 0, modelId, behavior, x, y, z, 0, 0, 0);
    CustomItem::SetBehavior(object, modelId, randoCheckId);

    if (param != NULL) {
        object->oBehParams2ndByte = param;
    }

    spawnedRandoObjects.insert({ randoCheckId, object });
}
