#include "Spoiler.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <filesystem>

#include <libultraship/libultra/types.h>

std::vector<std::string> Rando::Spoiler::spoilerLogs;

// Resolved on first use rather than at load time. On Android the app directory
// comes from SDL_AndroidGetExternalStoragePath(), which calls into Java — and a
// global constructor runs at dlopen, before SDL's JNI is set up, so asking then
// aborts the process with "CallStaticObjectMethod received NULL jclass".
static const std::filesystem::path& RandomizerFolderPath() {
    static const std::filesystem::path path(Ship::Context::GetPathRelativeToAppDirectory("randomizer", appShortName));
    return path;
}

void Rando::Spoiler::RefreshSpoilerLogs() {
    Rando::Spoiler::spoilerLogs.clear();

    Rando::Spoiler::spoilerLogs.push_back("Generate New Seed");
    s32 spoilerFileIndex = -1;

    if (!std::filesystem::exists(RandomizerFolderPath())) {
        std::filesystem::create_directory(RandomizerFolderPath());
    }

    for (const auto& entry : std::filesystem::directory_iterator(RandomizerFolderPath())) {
        if (entry.is_regular_file()) {
            std::string fileName = entry.path().filename().string();

            Rando::Spoiler::spoilerLogs.push_back(fileName);

            // Check if the current file is the one set in the cvar
            if (fileName == CVarGetString("gRandoSettings.SpoilerFile", "")) {
                spoilerFileIndex = Rando::Spoiler::spoilerLogs.size() - 1;
            }
        }
    }

    if (spoilerFileIndex == -1) {
        CVarSetInteger("gRandoSettings.SpoilerFileIndex", 0);
        CVarSetString("gRandoSettings.SpoilerFile", "");
    } else {
        CVarSetInteger("gRandoSettings.SpoilerFileIndex", spoilerFileIndex);
    }
}