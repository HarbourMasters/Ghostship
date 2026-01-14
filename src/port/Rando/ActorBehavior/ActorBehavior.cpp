#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

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

// Entry point for the module, run once on game boot
void Rando::ActorBehavior::Init() {
    REGISTER_LISTENER(SpawnMacroObject, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnMacroObject* ev = (SpawnMacroObject*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        int16_t model = *(ev->model);

        LogOutSpawns("Macro", model, ev->posX, ev->posY, ev->posZ);

        // RandoCheckId randoCheckId = Rando::StaticData::GetCheckByLocation(ev->posX, ev->posY, ev->posZ);
        // if (randoCheckId != RC_UNKNOWN) {
        //     RandoItemId randoItemId = Rando::StaticData::GetShuffledRandoItem(randoCheckId);
        // 	if (randoItemId != RI_UNKNOWN) {
        //         int16_t modelId = Rando::StaticData::GetModelByRandoItem(randoItemId);
        //         *(ev->model) = modelId;
        //         *(ev->behavior) = Rando::StaticData::GetBehaviorByModel(modelId);
        // 	}
        // }
    });

    REGISTER_LISTENER(SpawnObject, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnObject* ev = (SpawnObject*)event;
        struct SpawnInfo* info = *(ev->spawnInfo);

        uint32_t model = *(ev->model);

        LogOutSpawns("Spawn", model, info->startPos[0], info->startPos[1], info->startPos[2]);

        // RandoCheckId randoCheckId =
        //     Rando::StaticData::GetCheckByLocation(info->startPos[0], info->startPos[1], info->startPos[2]);
        // if (randoCheckId != RC_UNKNOWN) {
        //     RandoItemId randoItemId = Rando::StaticData::GetShuffledRandoItem(randoCheckId);
        //     if (randoItemId != RI_UNKNOWN) {
        //         int16_t modelId = Rando::StaticData::GetModelByRandoItem(randoItemId);
        //         *(ev->model) = modelId;
        //         info->behaviorScript = (void*)Rando::StaticData::GetBehaviorByModel(modelId);
        //         *(ev->spawnInfo) = info;
        //         event->cancelled = true;
        //     }
        // }
    });

    REGISTER_LISTENER(SpawnStar, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnStar* ev = (SpawnStar*)event;

        LogOutSpawns("Star", MODEL_STAR, ev->posX, ev->posY, ev->posZ);
    });
}
