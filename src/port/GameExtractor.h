#pragma once

#include "Companion.h"
#include <filesystem>
#include <vector>
#include <cstdint>

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
