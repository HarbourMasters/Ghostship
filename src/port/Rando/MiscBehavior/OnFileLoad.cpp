#include "MiscBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include "port/Rando/Spoiler/Spoiler.h"
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
        selectedFileNum = ev->fileNum - 1;

        if (!CVarGetInteger("gRandoSettings.Enabled", 0)) {
            gSaveBuffer.files[selectedFileNum]->shipSaveData.saveType = SAVETYPE_VANILLA;
            Rando::Logic::shuffledPool.clear();
            return;
        }

        if (!IS_RANDO(selectedFileNum)) {
            gSaveBuffer.files[selectedFileNum]->shipSaveData.saveType = SAVETYPE_RANDO;
            Rando::Logic::InitializeSaveChecks();

            if (CVarGetInteger("gRandoSettings.UseExistingLog", 0)) {
                std::string fileName = std::to_string(selectedFileNum) + ".json";
                bool logExists = SpoilerExistsForFileNum(fileName);
                if (!logExists) {
                    Rando::Logic::GenerateShuffleList();
                } else {
                    nlohmann::json loadedSpoiler = Rando::Spoiler::LoadFromFile(fileName);
                    Rando::Logic::shuffledPool = Rando::Spoiler::GenerateFromSpoilerLog(loadedSpoiler);
                }
            } else {
                Rando::Logic::GenerateShuffleList();
            }

            for (auto& pool : Rando::Logic::shuffledPool) {
                RandoSaveCheck randoSaveCheck;
                randoSaveCheck.randoItemId = pool.randoItemId;
                randoSaveCheck.randoAct = pool.randoAct;
                randoSaveCheck.obtained = pool.obtained;
                randoSaveCheck.skipped = pool.skipped;

                RANDO_SAVE_CHECKS(selectedFileNum)[pool.randoCheckId] = randoSaveCheck;
            }
            Notification::Emit(
                { .message = "Spoiler written to Save File.", .messageColor = ImVec4(0, 0.85f, 0.3f, 1) });
            save_file_do_save(selectedFileNum);
        } else {
            Rando::Logic::shuffledPool.clear();
            for (size_t i = 0; i < RC_MAX; i++) {
                RandoSaveCheck randoSaveCheck = RANDO_SAVE_CHECKS(selectedFileNum)[i];
                LevelShuffleEntry entry;
                entry.randoCheckId = (RandoCheckId)i;
                entry.randoItemId = randoSaveCheck.randoItemId;
                entry.randoAct = randoSaveCheck.randoAct;
                entry.obtained = randoSaveCheck.obtained;
                Rando::Logic::shuffledPool.push_back(entry);
            }
        }
    });
}