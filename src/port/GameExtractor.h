#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>
#include <optional>

namespace fs = std::filesystem;

class GameExtractor {
public:
    static bool GenAssetFile();
    std::optional<std::string> ValidateChecksum() const;
    bool SelectGameFromUI();
    void GetRoms(std::vector<std::string>& roms);
    bool GenerateOTR();
    void WritePortVersion();
private:
    fs::path mGamePath;
    std::vector<uint8_t> mGameData;
};
