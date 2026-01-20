#include "ObjectBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "port/hooks/list/PlayerEvent.h"
#include "port/hooks/list/EngineEvent.h"

extern "C" {
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

Rando::StaticData::RandoStaticCheck GetShuffledRandoStaticCheck(s16 x, s16 y, s16 z) {
    Rando::StaticData::RandoStaticCheck randoStaticCheck;
    RandoCheckId randoCheckId = Rando::StaticData::GetCheckByLocation(x, y, z);
    int16_t levelId = Rando::StaticData::Checks[randoCheckId].levelId;

    randoStaticCheck = Rando::StaticData::Checks[randoCheckId];
    randoStaticCheck.randoItemId = Rando::StaticData::GetShuffledRandoItem(levelId - 1, randoCheckId);
    randoStaticCheck.actData = Rando::StaticData::GetShuffledRandoAct(levelId - 1, randoCheckId);

    return randoStaticCheck;
}

void ModifySpawnedObject(bool* shouldCancel, s16 x, s16 y, s16 z, s32 param) {
    Rando::StaticData::RandoStaticCheck randoStaticCheck = GetShuffledRandoStaticCheck(x, y, z);
    if (!Rando::StaticData::IsCheckShuffled(Rando::StaticData::Checks[randoStaticCheck.randoCheckId].levelId - 1,
                                            randoStaticCheck.randoCheckId) ||
        randoStaticCheck.randoCheckId == RC_UNKNOWN || randoStaticCheck.randoItemId == RI_UNKNOWN) {
        return;
    }

    int32_t modelId = Rando::StaticData::GetModelByRandoItem(randoStaticCheck.randoItemId);
    const BehaviorScript* behavior =
        modelId == MODEL_BLUE_COIN ? bhvHiddenBlueCoin : Rando::StaticData::GetBehaviorByModel(modelId);

    CustomItem::SpawnObject(modelId, behavior, x, y, z, param, randoStaticCheck.randoCheckId, randoStaticCheck.actData);
    *(shouldCancel) = true;
}

// Entry point for the module, run once on game boot
void Rando::ObjectBehavior::Init() {
    REGISTER_LISTENER(ItemCollected, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ItemCollected* ev = (ItemCollected*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
        if (ev->object->unused1 != RC_UNKNOWN) {
            event->cancelled = true;
            CustomItem::ObjectCollected(ev->type, ev->marioState, ev->object);
        }
    });

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

    REGISTER_LISTENER(ModifyObjectBehavior, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ModifyObjectBehavior* ev = (ModifyObjectBehavior*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        event->cancelled = true;

        switch (ev->model) {
            case MODEL_BLUE_COIN:
                // Simply here for Skipping the 200 Frame timeout.
                break;
            case MODEL_BLUE_COIN_SWITCH:
                Rando::ObjectBehavior::ModifyBlueCoinSwitchBehavior();
                break;
            case MODEL_RED_COIN:
                Rando::ObjectBehavior::ModifyRedCoinBehavior(&event->cancelled, ev->object);
                break;
            default:
                event->cancelled = false;
                break;
        }
    });

    REGISTER_LISTENER(LevelScriptExecute, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptExecute* ev = (LevelScriptExecute*)event;
        if (isInitialized || ev->command != 36) {
            return;
        }

        if (!isInitialized) {
            LOAD_MODEL_FROM_GEO(MODEL_STAR, star_geo);
            LOAD_MODEL_FROM_GEO(MODEL_RED_COIN, red_coin_geo);
            LOAD_MODEL_FROM_GEO(MODEL_BLUE_COIN, blue_coin_geo);
            isInitialized = true;
        }
    });
}
