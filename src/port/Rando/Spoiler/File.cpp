#include "Spoiler.h"
#include "port/ui/Notification.h"
#include <fstream>
#include <filesystem>

const std::string appShortName = "sm64";
namespace fs = std::filesystem;

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
    nlohmann::json spoiler;
    std::string spoilerFilePath = Ship::Context::GetPathRelativeToAppDirectory("randomizer/" + fileName, appShortName);
    std::ifstream fileStream(spoilerFilePath);

    if (!fs::exists(spoilerFilePath)) {
        return spoiler;
    }

    if (!fileStream.is_open()) {
        Notification::Emit(
            { .message = "Error: Failed to open spoiler file.", .messageColor = ImVec4(0.85f, 0.3f, 0, 1) });
        return spoiler;
    }

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
