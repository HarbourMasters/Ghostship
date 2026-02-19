#ifndef RANDO_SPOILER_H
#define RANDO_SPOILER_H

#include <vector>
#include <string>
#include "nlohmann/json.hpp"
#include "port/Rando/Logic/Logic.h"
#include <filesystem>

const std::string appShortName = "sm64";

namespace Rando {

namespace Spoiler {
extern std::vector<std::string> spoilerLogs;

nlohmann::json GenerateFromPoolGeneration(std::vector<LevelShuffleEntry>& shuffledPool,
                                          std::vector<RandoSaveEntrance>& shuffledEntrances);

void GenerateFromSpoiler(nlohmann::json spoiler);
void RefreshSpoilerLogs();
void SaveToFile(const std::string& fileName, nlohmann::json spoiler);
nlohmann::json LoadFromFile(const std::string& filePath);
// void ApplyToSaveContext(nlohmann::json spoiler);
// bool HandleFileDropped(char* path);

} // namespace Spoiler

} // namespace Rando

#endif