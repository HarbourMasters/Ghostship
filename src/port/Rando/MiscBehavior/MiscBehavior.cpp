#include "MiscBehavior.h"

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
}
