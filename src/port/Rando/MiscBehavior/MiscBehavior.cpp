#include "MiscBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include "port/hooks/list/EngineEvent.h"

extern "C" {
#include "game/ingame_menu.h"
#include "game/level_update.h"
void initiate_warp(s16 destLevel, s16 destArea, s16 destWarpNode, s32 arg3);
}

static RandoEntranceId currentEntrance = RE_UNKNOWN;
static RandoEntranceId shuffledEntrance = RE_UNKNOWN;

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
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }
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

        if (ev->sourceWarpNode >= 241 && currentEntrance != RE_UNKNOWN) {
            Rando::StaticData::RandoStaticEntrance randoStaticEntrance = Rando::StaticData::Entrances[currentEntrance];

            ev->warpNode->destNode = randoStaticEntrance.deathWarpId;
            ev->warpNode->destArea = randoStaticEntrance.deathArea;

            switch (currentEntrance) {
                case RE_BBH:
                    ev->warpNode->destLevel = LEVEL_CASTLE_COURTYARD;
                    break;
                case RE_VCUTM:
                    ev->warpNode->destLevel = LEVEL_CASTLE_GROUNDS;
                    break;
                default:
                    ev->warpNode->destLevel = LEVEL_CASTLE;
                    break;
            }

            currentEntrance = RE_UNKNOWN;
            shuffledEntrance = RE_UNKNOWN;
        } else {
            RandoEntranceId randoEntranceId;
            for (auto& [randoEntranceId, randoStaticEntrance] : Rando::StaticData::Entrances) {
                if (randoStaticEntrance.destinationId == ev->warpNode->destLevel) {
                    // TODO: Handle Tiny Huge Islands size changes, Wet Dry World water level, and Tick Tock Clocks
                    // clock.
                    ev->warpNode->destLevel = RANDO_SAVE_ENTRANCES(selectedFileNum)[randoEntranceId].destinationId;
                    for (auto& entrance : Rando::Logic::shuffledEntrances) {
                        if (entrance.randoEntranceId == randoEntranceId) {
                            entrance.found = true;
                            save_file_do_save(selectedFileNum);
                        }
                    }
                    currentEntrance = randoEntranceId;
                    shuffledEntrance = Rando::StaticData::GetEntranceIdFromDestination(ev->warpNode->destLevel);
                    break;
                }
            }
        }
    });

    REGISTER_LISTENER(ExitLevel, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        ExitLevel* ev = (ExitLevel*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        if (ev->menuOption == MENU_OPT_EXIT_COURSE) {
            currentEntrance = RE_UNKNOWN;
        }
    });
}
