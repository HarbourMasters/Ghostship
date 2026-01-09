#ifndef RANDO_H
#define RANDO_H

// #include "StaticData/StaticData.h"
#include "Types.h"

#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
// #define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
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
void RegisterMenu();

} // namespace Rando

#endif
