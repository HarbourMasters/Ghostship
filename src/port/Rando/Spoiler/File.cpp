#include "Spoiler.h"
#include <fstream>

const std::string appShortName = "sm64";

namespace Rando {

namespace Spoiler {

void SaveToFile(const std::string& fileName, nlohmann::json spoiler) {
    std::string filePath = Ship::Context::GetPathRelativeToAppDirectory("randomizer/" + fileName, appShortName);
    std::ofstream fileStream(filePath);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Failed to open spoiler file");
    }

    fileStream << spoiler.dump(4);
}

nlohmann::json LoadFromFile(const std::string& fileName) {
    std::string spoilerFilePath = Ship::Context::GetPathRelativeToAppDirectory("randomizer/" + fileName, appShortName);
    std::ifstream fileStream(spoilerFilePath);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Failed to open spoiler file");
    }

    nlohmann::json spoiler;
    try {
        fileStream >> spoiler;
    } catch (nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse spoiler file: " + std::string(e.what()));
    }

    if (!spoiler.contains("type") || spoiler["type"] != "GHOSTSHIP_RANDO_SPOILER") {
        throw std::runtime_error("Spoiler file is not a valid spoiler file");
    }

    return spoiler;
}

} // namespace Spoiler

} // namespace Rando
