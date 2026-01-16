#include "CustomItem.h"

extern "C" {
#include "game/object_list_processor.h"
#include "include/behavior_data.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
}

void CustomItem::Spawn(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z) {
    spawn_object_abs_with_rot(&gMacroObjectDefaultParent, 0, modelId, behavior, x, y, z, 0, 0, 0);
}
