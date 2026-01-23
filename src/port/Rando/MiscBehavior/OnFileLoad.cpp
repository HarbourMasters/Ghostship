#include "MiscBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/ui/Notification.h"

extern "C" {
extern struct SaveBuffer gSaveBuffer;
}

bool SpoilerExistsForFileNum(std::string fileName) {
    nlohmann::json spoilerCheck = Rando::Spoiler::LoadFromFile(fileName);
    if (spoilerCheck.empty()) {
        Notification::Emit({ .message = "Error: No Spoiler Log found.", .messageColor = ImVec4(0.85f, 0.3f, 0, 1) });
        return false;
    } else {
        return true;
    }
}

void Rando::MiscBehavior::OnFileLoad() {
    REGISTER_LISTENER(OnGameFileLoad, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        OnGameFileLoad* ev = (OnGameFileLoad*)event;
        if (!CVarGetInteger("gRandoSettings.Enabled", 0)) {
            gSaveBuffer.files[ev->fileNum - 1]->shipSaveData.saveType = SAVETYPE_VANILLA;
            return;
        }

        selectedFileNum = ev->fileNum - 1;
        std::string fileName = std::to_string(selectedFileNum) + ".json";
        bool logExists = SpoilerExistsForFileNum(fileName);

        if (!IS_RANDO(selectedFileNum)) {
            gSaveBuffer.files[selectedFileNum]->shipSaveData.saveType = SAVETYPE_RANDO;
            if (!logExists) {
                Rando::Logic::GenerateShuffleList();
            } else {
                nlohmann::json loadedSpoiler = Rando::Spoiler::LoadFromFile(fileName);
                Rando::Logic::shuffledPool = Rando::Spoiler::GenerateFromSpoilerLog(loadedSpoiler);
            }
            Rando::CheckTracker::SetCheckTrackerList();
        }

        // TODO: Inject Save File with spoiler data
        // gSaveBuffer.files[ev->fileNum]->shipSaveData.randoSaveData.isRando = true;

        // bcopy(&gSaveBuffer.files[ev->fileNum][0], &gSaveBuffer.files[ev->fileNum][1],
        //       sizeof(gSaveBuffer.files[ev->fileNum][1]));

        // write_eeprom_data(&gSaveBuffer.menuData[ev->fileNum], sizeof(gSaveBuffer.menuData[ev->fileNum]));
    });
}