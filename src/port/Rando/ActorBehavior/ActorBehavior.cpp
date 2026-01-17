#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "port/hooks/list/PlayerEvent.h"
#include "port/hooks/list/EngineEvent.h"

extern "C" {
#include "game/spawn_object.h"
#include "include/behavior_data.h"
#include "include/level_commands.h"
#include "assets/actors/star/geo.h"
#include "assets/actors/coin/geo.h"
}

static bool isInitialized = false;

void LogOutSpawns(std::string type, int16_t model, int16_t posX, int16_t posY, int16_t posZ) {
    if (model != MODEL_STAR && model != MODEL_RED_COIN && model != MODEL_RED_COIN_NO_SHADOW &&
        model != MODEL_BLUE_COIN && model != MODEL_BLUE_COIN_NO_SHADOW) {
        return;
    }
    std::string locationStr = std::to_string(posX) + ", " + std::to_string(posY) + ", " + std::to_string(posZ);
    SPDLOG_INFO("Type: {} | Model: {} | Position: {}", type, model, locationStr);
}

Rando::StaticData::RandoCustomData GetRandoData(s16 x, s16 y, s16 z) {
    Rando::StaticData::RandoCustomData randoCustomData;
    randoCustomData.randoCheckId = Rando::StaticData::GetCheckByLocation(x, y, z);
    randoCustomData.randoItemId = Rando::StaticData::GetShuffledRandoItem(randoCustomData.randoCheckId);
    randoCustomData.isShuffled = Rando::StaticData::IsCheckShuffled(randoCustomData.randoCheckId);

    return randoCustomData;
}

void ModifySpawnedObject(bool* shouldCancel, s16 x, s16 y, s16 z, s32 param) {
    Rando::StaticData::RandoCustomData randoCustomData = GetRandoData(x, y, z);
    if (!randoCustomData.isShuffled || randoCustomData.randoCheckId == RC_UNKNOWN ||
        randoCustomData.randoItemId == RI_UNKNOWN) {
        *(shouldCancel) = false;
        return;
    }

    int32_t modelId = Rando::StaticData::GetModelByRandoItem(randoCustomData.randoItemId);
    const BehaviorScript* behavior = Rando::StaticData::GetBehaviorByModel(modelId);

    CustomItem::SpawnObject(modelId, behavior, x, y, z, param);
    *(shouldCancel) = true;
}

// Entry point for the module, run once on game boot
void Rando::ActorBehavior::Init() {
    REGISTER_LISTENER(SpawnObject, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnObject* ev = (SpawnObject*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        LogOutSpawns("Object", (int16_t)ev->model, ev->posX, ev->posY, ev->posZ);
        ModifySpawnedObject(&event->cancelled, ev->posX, ev->posY, ev->posZ, NULL);
    });

    REGISTER_LISTENER(SpawnStar, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnStar* ev = (SpawnStar*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        if (*(ev->model) != MODEL_STAR) {
            return;
        }

        LogOutSpawns("Star", MODEL_STAR, ev->posX, ev->posY, ev->posZ);
        ModifySpawnedObject(&event->cancelled, ev->posX, ev->posY, ev->posZ, NULL);
    });

    REGISTER_LISTENER(ModifyDefaultStar, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ModifyDefaultStar* ev = (ModifyDefaultStar*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        LogOutSpawns("Default Star", MODEL_STAR, ev->posX, ev->posY, ev->posZ);
        ModifySpawnedObject(&event->cancelled, ev->posX, ev->posY, ev->posZ, ev->param);
    });

    REGISTER_LISTENER(LevelScriptExecute, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptExecute* ev = (LevelScriptExecute*)event;
        if (isInitialized || ev->command != 36) {
            return;
        }

        if (!isInitialized) {
            LOAD_MODEL_FROM_GEO(MODEL_STAR, star_geo);
            LOAD_MODEL_FROM_GEO(MODEL_RED_COIN, red_coin_geo);
            isInitialized = true;
        }
    });
}
