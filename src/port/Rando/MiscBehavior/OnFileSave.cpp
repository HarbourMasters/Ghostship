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

        for (auto& pool : Rando::Logic::shuffledPool) {
            RandoSaveCheck randoSaveCheck;
            randoSaveCheck.randoItemId = pool.randoItemId;
            randoSaveCheck.randoAct = pool.randoAct;
            randoSaveCheck.obtained = pool.obtained;

            RANDO_SAVE_CHECKS(selectedFileNum)[pool.randoCheckId] = randoSaveCheck;
        }

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;

        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));

        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    });
}