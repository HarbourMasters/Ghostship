#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>
#include <optional>
#include <atomic>

namespace fs = std::filesystem;

class GameExtractor {
public:
    static bool GenAssetFile();
    std::optional<std::string> ValidateChecksum() const;
    bool RunStandalone(std::string rom);
    bool SelectGameFromUI();
    void SetSearchPath(const std::string& path);
    void GetRoms(std::vector<std::string>& roms);
    std::string GetRomPath();
    bool Parse(std::atomic<size_t>& assetCount, std::string appShortName = "");
    bool GenerateOTR(std::string appShortName = "");
    bool GenerateOTR(std::atomic<size_t>& assetCount, std::string appShortName = "");
    void WritePortVersion();
private:
    fs::path mGamePath;
    std::vector<uint8_t> mGameData;
    std::string mSearchPath;
};
