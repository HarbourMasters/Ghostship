#ifndef RANDO_SPOILER_H
#define RANDO_SPOILER_H

#include <vector>
#include <string>
#include "nlohmann/json.hpp"
#include "port/Rando/Logic/Logic.h"

namespace Rando {

namespace Spoiler {
// extern std::vector<std::string> spoilerOptions;
// void RefreshOptions();

nlohmann::json GenerateFromPoolGeneration(std::vector<LevelShuffleEntry>& shuffledPool);
std::vector<LevelShuffleEntry> GenerateFromSpoilerLog(nlohmann::json spoiler);
void SaveToFile(const std::string& fileName, nlohmann::json spoiler);
nlohmann::json LoadFromFile(const std::string& filePath);
// void ApplyToSaveContext(nlohmann::json spoiler);
// bool HandleFileDropped(char* path);

} // namespace Spoiler

} // namespace Rando

#endif