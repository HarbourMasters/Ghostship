#include "MiscBehavior.h"

extern "C" {
extern struct SaveBuffer gSaveBuffer;
}

void Rando::MiscBehavior::OnFileLoad() {
    REGISTER_LISTENER(OnGameFileLoad, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnGameFileLoad* ev = (OnGameFileLoad*)event;
        if (!CVarGetInteger("gRandoSettings.Enabled", 0)) {
            gSaveBuffer.files[ev->fileNum - 1]->shipSaveData.saveType = SAVETYPE_VANILLA;
            return;
        }

        selectedFileNum = ev->fileNum - 1;

        if (!IS_RANDO(selectedFileNum)) {
            gSaveBuffer.files[selectedFileNum]->shipSaveData.saveType = SAVETYPE_RANDO;
            // Rando::Logic::GenerateShuffleList();
        }
        Rando::Logic::GenerateShuffleList();

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;

        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));

        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    });
}