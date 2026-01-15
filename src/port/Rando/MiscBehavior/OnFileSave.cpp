#include "MiscBehavior.h"

void Rando::MiscBehavior::OnFileSave() {
    REGISTER_LISTENER(OnGameFileSave, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnGameFileSave* ev = (OnGameFileSave*)event;
        if (!IS_RANDO(selectedFileNum)) {
            return;
        }

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;

        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));

        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    });
}