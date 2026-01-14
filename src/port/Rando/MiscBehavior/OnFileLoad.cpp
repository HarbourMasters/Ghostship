#include "MiscBehavior.h"

extern "C" {
extern struct SaveBuffer gSaveBuffer;
}

void Rando::MiscBehavior::OnFileLoad() {
    REGISTER_LISTENER(OnGameFileLoad, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnGameFileLoad* ev = (OnGameFileLoad*)event;
        if (!IS_RANDO) {
            return;
        }
        SPDLOG_INFO("File Loaded");
        if (!gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando) {
            gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;
            Rando::StaticData::ShuffleItemList();
        }

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;
    
        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));
    
        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    
    });
}