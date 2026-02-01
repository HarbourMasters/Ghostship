#include "MiscBehavior.h"
#include "port/hooks/list/EngineEvent.h"

extern "C" {
#include "game/ingame_menu.h"
#include "game/level_update.h"
void initiate_warp(s16 destLevel, s16 destArea, s16 destWarpNode, s32 arg3);
}

// Entry point for the module, run once on game boot
void Rando::MiscBehavior::Init() {
    Rando::MiscBehavior::OnFileLoad();
    Rando::MiscBehavior::OnFileSave();

    REGISTER_LISTENER(ModifyRedCoinCount, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ModifyRedCoinCount* ev = (ModifyRedCoinCount*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        *(ev->redCoinsCollected) = CustomItem::redCoinsCollected;
        event->cancelled = true;
    });

    REGISTER_LISTENER(LevelScriptExecute, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptExecute* ev = (LevelScriptExecute*)event;
        if (ev->command == 17) {
            if (gCurrLevelNum == LEVEL_CASTLE || gCurrLevelNum == LEVEL_CASTLE_COURTYARD ||
                gCurrLevelNum == LEVEL_CASTLE_GROUNDS) {
                gRedCoinsCollected = 0;
                CustomItem::redCoinsCollected = 0;
                CustomItem::ClearSpawnedObjects();
            }
        }
    });

    REGISTER_LISTENER(ChangeLevel, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ChangeLevel* ev = (ChangeLevel*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        SPDLOG_INFO("Source: {}", std::to_string(ev->sourceWarpNode));
        SPDLOG_INFO("Current Destination: {}", std::to_string(ev->warpNode->destLevel));
        SPDLOG_INFO("Current Level: {}", std::to_string(ev->warpNode->destLevel));

        RandoEntranceId randoEntranceId;
        for (auto& [randoEntranceId, randoStaticEntrance] : Rando::StaticData::Entrances) {
            if (randoStaticEntrance.destinationId == ev->warpNode->destLevel) {
                // TODO: Death Exits
                if (ev->sourceWarpNode > 0) {
                    return;
                } else {
                    // TODO: Handle Tiny Huge Islands size changes, Wet Dry World water level, and Tick Tock Clocks
                    // clock.
                    ev->warpNode->destLevel = RANDO_SAVE_ENTRANCES(selectedFileNum)[randoEntranceId].destinationId;
                    SPDLOG_INFO("New Level: {}", std::to_string(ev->warpNode->destLevel));
                }
                break;
            }
        }
    });
}
