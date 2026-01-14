#ifndef RANDO_H
#define RANDO_H

#include "StaticData/StaticData.h"
#include "Types.h"
#include "include/model_ids.h"
#include "include/behavior_data.h"
#include "include/object_fields.h"
#include "include/object_constants.h"
#include "game/save_file.h"

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

extern "C" {
#include "sm64.h"
#include "game/object_list_processor.h"
#include "game/macro_special_objects.h"
#include "game/object_helpers.h"
}

#define IS_RANDO CVarGetInteger("gRandoSettings.Enabled", 0)
#define RANDO_SAVE_CHECKS(fileNum) gSaveBuffer.files[fileNum]->shipSaveData.randoSaveData.randoSaveChecks
// #define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions
// #define RANDO_EVENTS gSaveContext.save.shipSaveInfo.rando.randoEvents
// #define RANDO_STARTING_ITEMS gSaveContext.save.shipSaveInfo.rando.randoStartingItems

namespace Rando {

void Init();
// void DrawItem(RandoItemId randoItemId, Actor* actor = nullptr);
// void GiveItem(RandoItemId randoItemId);
// void RemoveItem(RandoItemId randoItemId);
// RandoItemId CurrentJunkItem();
// bool IsItemObtainable(RandoItemId randoItemId, RandoCheckId randoCheckId = RC_UNKNOWN);
// RandoItemId ConvertItem(RandoItemId randoItemId, RandoCheckId randoCheckId = RC_UNKNOWN);
// RandoCheckId FindItemPlacement(RandoItemId randoItemId);
} // namespace Rando

#endif
