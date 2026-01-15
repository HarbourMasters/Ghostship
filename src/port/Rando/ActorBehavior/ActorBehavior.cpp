#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"
#include "include/behavior_data.h"

// TODO: Need to possibly set behaviorArg to fix Star on Red Coin spawn behavior.
// - Star will spin on an alternate axis and be uncollectable.

void LogOutSpawns(std::string type, int16_t model, int16_t posX, int16_t posY, int16_t posZ) {
    if (model != MODEL_STAR && model != MODEL_RED_COIN && model != MODEL_RED_COIN_NO_SHADOW &&
        model != MODEL_BLUE_COIN && model != MODEL_BLUE_COIN_NO_SHADOW) {
        return;
    }
    std::string locationStr = std::to_string(posX) + ", " + std::to_string(posY) + ", " + std::to_string(posZ);
    SPDLOG_INFO("Type: {} | Model: {} | Position: {}", type, model, locationStr);
}

void ModifySpawnedObject(bool* shouldCancel, struct SpawnInfo* spawnInfo) {
    RandoCheckId randoCheckId =
        Rando::StaticData::GetCheckByLocation(spawnInfo->startPos[0], spawnInfo->startPos[1], spawnInfo->startPos[2]);
    if (randoCheckId == RC_UNKNOWN || !Rando::StaticData::IsCheckShuffled(randoCheckId)) {
        *(shouldCancel) = false;
        return;
    }

    RandoItemId randoItemId = Rando::StaticData::GetShuffledRandoItem(randoCheckId);
    if (randoItemId != RI_UNKNOWN) {
        spawnInfo->behaviorScript = segmented_to_virtual(
            Rando::StaticData::GetBehaviorByModel(Rando::StaticData::GetModelByRandoItem(randoItemId)));
        spawnInfo->model = NULL;
        spawnInfo->behaviorArg = (u32)randoItemId;
    }
    CustomItem::Spawn(spawnInfo);
    *(shouldCancel) = true;
}

// Entry point for the module, run once on game boot
void Rando::ActorBehavior::Init() {
    REGISTER_LISTENER(SpawnObject, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnObject* ev = (SpawnObject*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        LogOutSpawns("Object", (int16_t)ev->spawnInfo->model, ev->spawnInfo->startPos[0], ev->spawnInfo->startPos[1],
                     ev->spawnInfo->startPos[2]);
        ModifySpawnedObject(&event->cancelled, ev->spawnInfo);
    });

    REGISTER_LISTENER(SpawnStar, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnStar* ev = (SpawnStar*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        LogOutSpawns("Star", MODEL_STAR, ev->posX, ev->posY, ev->posZ);
    });
}
