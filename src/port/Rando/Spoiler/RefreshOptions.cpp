#include "Spoiler.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <filesystem>

#include <libultraship/libultra/types.h>

std::vector<std::string> Rando::Spoiler::spoilerLogs;
const std::filesystem::path randomizerFolderPath(Ship::Context::GetPathRelativeToAppDirectory("randomizer",
                                                                                              appShortName));

void Rando::Spoiler::RefreshSpoilerLogs() {
    Rando::Spoiler::spoilerLogs.clear();

    Rando::Spoiler::spoilerLogs.push_back("Generate New Seed");
    s32 spoilerFileIndex = -1;

    if (!std::filesystem::exists(randomizerFolderPath)) {
        std::filesystem::create_directory(randomizerFolderPath);
    }

    for (const auto& entry : std::filesystem::directory_iterator(randomizerFolderPath)) {
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