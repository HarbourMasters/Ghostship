#include "SaveModData.h"

#include <array>

#include <libultraship.h>
extern "C" {
#include "game/area.h"
#include "game/save_file.h"
}

namespace {

struct FileData {
    nlohmann::json disk = nlohmann::json::object();
    nlohmann::json live = nlohmann::json::object();
};

std::array<FileData, NUM_SAVE_FILES> sFiles;
// What Current() hands out outside a valid file, so a mod never writes into another file by mistake.
nlohmann::json sScratch = nlohmann::json::object();

bool Valid(int32_t fileIndex) {
    return fileIndex >= 0 && fileIndex < NUM_SAVE_FILES;
}

} // namespace

void SaveModData::Load(int32_t fileIndex, const nlohmann::json& mods) {
    if (!Valid(fileIndex)) {
        return;
    }
    sFiles[fileIndex].disk = mods.is_object() ? mods : nlohmann::json::object();
    sFiles[fileIndex].live = sFiles[fileIndex].disk;
}

const nlohmann::json& SaveModData::Commit(int32_t fileIndex) {
    if (!Valid(fileIndex)) {
        return sScratch;
    }
    sFiles[fileIndex].disk = sFiles[fileIndex].live;
    return sFiles[fileIndex].disk;
}

nlohmann::json& SaveModData::Current() {
    const int32_t fileIndex = gCurrSaveFileNum - 1;
    if (!Valid(fileIndex)) {
        sScratch = nlohmann::json::object();
        return sScratch;
    }
    return sFiles[fileIndex].live;
}

extern "C" void SaveModData_Erase(int32_t fileIndex) {
    if (Valid(fileIndex)) {
        sFiles[fileIndex].live = nlohmann::json::object();
    }
}

extern "C" void SaveModData_Copy(int32_t srcFileIndex, int32_t destFileIndex) {
    if (Valid(srcFileIndex) && Valid(destFileIndex)) {
        // Like save_file_copy, from the file as it is in memory.
        sFiles[destFileIndex].live = sFiles[srcFileIndex].live;
    }
}

extern "C" void SaveModData_Revert(int32_t fileIndex) {
    if (Valid(fileIndex)) {
        sFiles[fileIndex].live = sFiles[fileIndex].disk;
    }
}
