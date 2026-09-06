#include "Saves.h"

#include "sm64.h"
#include "port/ShipInit.hpp"
#include "port/data/SaveConversion.h"

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

extern "C" struct SaveBuffer gSaveBuffer;

// Resolved on first use rather than at load time. On Android the app directory
// comes from SDL_AndroidGetExternalStoragePath(), which calls into Java — and a
// global constructor runs at dlopen, before SDL's JNI is set up, so asking then
// aborts the process with "CallStaticObjectMethod received NULL jclass".
static const fs::path& SavesPath() {
    static const fs::path path(Ship::Context::GetPathRelativeToAppDirectory("saves", "sm64"));
    return path;
}

static void Init() {
    // Create saves directory if it doesn't exist
    if (!fs::exists(SavesPath())) {
        fs::create_directory(SavesPath());
    }
}

extern "C" {
void RestoreMainMenuData(int32_t srcSlot) {
    int32_t destSlot = srcSlot ^ 1;

    bcopy(&gSaveBuffer.menuData[srcSlot], &gSaveBuffer.menuData[destSlot], sizeof(gSaveBuffer.menuData[destSlot]));
}

void RestoreSaveFileData(int32_t fileIndex, int32_t srcSlot) {
    int32_t destSlot = srcSlot ^ 1;

    bcopy(&gSaveBuffer.files[fileIndex][srcSlot], &gSaveBuffer.files[fileIndex][destSlot],
          sizeof(gSaveBuffer.files[fileIndex][destSlot]));
}

void SaveFileDoSave(int32_t fileIndex) {
    std::ofstream file(SavesPath() / ("save_" + std::to_string(fileIndex) + ".json"), std::ios::out);
    if (!file.is_open()) {
        return;
    }

    json j = gSaveBuffer.files[fileIndex][0];
    file << j.dump(1);
    file.close();
}

bool ShouldLoadOldSaveFile(void) {
    return fs::exists(Ship::Context::GetPathRelativeToAppDirectory("default.sav"));
}

// save_file_reload() (the game-over path) restores each save from its in-memory
// backup slot [1], but this loader only fills slot [0], leaving the backups
// zeroed. A game over then copies zeros over the live save, which shows as a
// wiped file until restart and becomes permanent once anything saves. Mirror
// the loaded data into the backup slots so reload restores what was loaded.
static void MirrorSavesToBackupSlots(void) {
    for (int32_t fileIndex = 0; fileIndex < NUM_SAVE_FILES; fileIndex++) {
        gSaveBuffer.files[fileIndex][1] = gSaveBuffer.files[fileIndex][0];
    }
    gSaveBuffer.menuData[1] = gSaveBuffer.menuData[0];
}

void SaveFileLoadAll(void) {
    auto oldSave = Ship::Context::GetPathRelativeToAppDirectory("default.sav");
    if (fs::exists(oldSave)) {
        for (int32_t fileIndex = 0; fileIndex < NUM_SAVE_FILES; fileIndex++) {
            SaveFileDoSave(fileIndex);
        }
        // Move old save files to backup
        fs::rename(oldSave, Ship::Context::GetPathRelativeToAppDirectory("default.sav.bak"));
        MirrorSavesToBackupSlots();
        return;
    }

    // Read save files
    for (int32_t fileIndex = 0; fileIndex < NUM_SAVE_FILES; fileIndex++) {
        fs::path filepath = SavesPath() / ("save_" + std::to_string(fileIndex) + ".json");
        if (!fs::exists(filepath)) {
            continue;
        }

        std::ifstream file(filepath, std::ios::in);
        if (!file.is_open()) {
            continue;
        }

        json j;
        file >> j;
        // Migrate Existing Saves to Include Vanilla ShipSaveData.
        if (!j.contains("shipSaveData")) {
            j["shipSaveData"] = json::object();
            j["shipSaveData"]["features"] = ShipSaveFeatures{ .achievements = false, .rando = false };
        }

        gSaveBuffer.files[fileIndex][0] = j.get<struct SaveFile>();
        file.close();
    }

    // Read global save file
    fs::path globalpath = SavesPath() / "global.json";
    if (fs::exists(globalpath)) {
        std::ifstream file(globalpath, std::ios::in);
        if (file.is_open()) {
            json j;
            file >> j;
            gSaveBuffer.menuData[0] = j.get<struct MainMenuSaveData>();
            file.close();
        }
    }

    MirrorSavesToBackupSlots();
}

void SaveMainMenuData(void) {
    std::ofstream file(SavesPath() / "global.json", std::ios::out);
    if (!file.is_open()) {
        return;
    }

    json j = gSaveBuffer.menuData[0];
    file << j.dump(1);
    file.close();
}
}

static RegisterShipInitFunc initFunc(Init);