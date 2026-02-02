#include "MiscBehavior.h"

#include "port/Rando/Logic/Logic.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/ui/Notification.h"

void Rando::MiscBehavior::OnFileSave() {
    REGISTER_LISTENER(OnGameFileSave, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnGameFileSave* ev = (OnGameFileSave*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        for (auto& check : Rando::Logic::shuffledPool) {
            RandoSaveCheck randoSaveCheck;
            randoSaveCheck.randoItemId = check.randoItemId;
            randoSaveCheck.randoAct = check.randoAct;
            randoSaveCheck.obtained = check.obtained;

            RANDO_SAVE_CHECKS(selectedFileNum)[check.randoCheckId] = randoSaveCheck;
        }

        for (auto& entrance : Rando::Logic::shuffledEntrances) {
            RANDO_SAVE_ENTRANCES(selectedFileNum)[entrance.randoEntranceId].found = entrance.found;
        }
    });
}