#include "MiscBehavior.h"

#include "game/save_file.h"
#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

extern "C" {
#include "sm64.h"
// extern s32 write_eeprom_data(void* buffer, s32 size);
}

extern struct SaveBuffer gSaveBuffer;

void Rando::MiscBehavior::OnFileLoad() {
    REGISTER_LISTENER(OnLoadSave, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnLoadSave* ev = (OnLoadSave*)event;
        if (!IS_RANDO) {
            return;
        }
        SPDLOG_INFO("File Loaded");
        Rando::StaticData::ShuffleItemList();

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;
    
        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));
    
        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    
    });
}