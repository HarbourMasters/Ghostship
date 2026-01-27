#include "MiscBehavior.h"
#include "port/hooks/list/EngineEvent.h"

extern "C" {
#include "game/ingame_menu.h"
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
}
