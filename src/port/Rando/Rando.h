#ifndef RANDO_H
#define RANDO_H

#include "Types.h"
#include "include/model_ids.h"
#include "include/behavior_data.h"
#include "include/object_fields.h"
#include "include/object_constants.h"
#include "game/save_file.h"
#include "port/Rando/CustomItem/CustomItem.h"

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"
#include "include/types.h"

extern "C" {
#include "sm64.h"
extern struct SaveBuffer gSaveBuffer;
}

#define IS_RANDO(fileNum) (gSaveBuffer.files[fileNum]->shipSaveData.features.rando)
#define RANDO_SAVE_CHECKS(fileNum) gSaveBuffer.files[fileNum]->shipSaveData.randoSaveData.randoSaveChecks
#define RANDO_SAVE_ENTRANCES(fileNum) gSaveBuffer.files[fileNum]->shipSaveData.randoSaveData.randoSaveEntrances
#define RANDO_SAVE_OPTIONS(fileNum) gSaveBuffer.files[fileNum]->shipSaveData.randoSaveData.randoSaveOptions
// #define RANDO_EVENTS gSaveContext.save.shipSaveInfo.rando.randoEvents
// #define RANDO_STARTING_ITEMS gSaveContext.save.shipSaveInfo.rando.randoStartingItems

extern int16_t selectedFileNum;

namespace Rando {

void Init();
// bool IsItemObtainable(RandoItemId randoItemId, RandoCheckId randoCheckId = RC_UNKNOWN);
} // namespace Rando

#endif // RANDO_H
