#include "Saves.h"

#include "sm64.h"
#include "port/ShipInit.hpp"
#include "port/data/SaveConversion.h"

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

extern "C" struct SaveBuffer gSaveBuffer;

static void Init() {
    // Create saves directory if it doesn't exist
    fs::path dir("saves");
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
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
    std::ofstream file(fs::path("saves/save_" + std::to_string(fileIndex) + ".json"), std::ios::out);
    if (!file.is_open()) {
        return;
    }

    json j = gSaveBuffer.files[fileIndex][0];
    file << j.dump(1);
    file.close();
}

bool ShouldLoadOldSaveFile(void) {
    fs::path save("default.sav");
    return fs::exists(save);
}

void SaveFileLoadAll(void) {
    fs::path save("default.sav");
    if (fs::exists(save)) {
        for (int32_t fileIndex = 0; fileIndex < NUM_SAVE_FILES; fileIndex++) {
            SaveFileDoSave(fileIndex);
        }
        // Move old save files to backup
        fs::rename(save, "default.sav.bak");
        return;
    }

    // Read save files
    for (int32_t fileIndex = 0; fileIndex < NUM_SAVE_FILES; fileIndex++) {
        fs::path filepath = fs::path("saves/save_" + std::to_string(fileIndex) + ".json");
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
    fs::path globalpath = fs::path("saves/global.json");
    if (fs::exists(globalpath)) {
        std::ifstream file(globalpath, std::ios::in);
        if (file.is_open()) {
            json j;
            file >> j;
            gSaveBuffer.menuData[0] = j.get<struct MainMenuSaveData>();
            file.close();
        }
    }
}

void SaveMainMenuData(void) {
    std::ofstream file(fs::path("saves/global.json"), std::ios::out);
    if (!file.is_open()) {
        return;
    }

    json j = gSaveBuffer.menuData[0];
    file << j.dump(1);
    file.close();
}
}

static RegisterShipInitFunc initFunc(Init);