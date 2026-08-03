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
    // Same as GenerateOTR, but with the two directories named outright instead of
    // asked of Ship::Context. The Android launcher extracts before SDL is up, so
    // Context cannot resolve a path there yet.
    bool GenerateOTRTo(std::atomic<size_t>& assetCount, const std::string& assetsPath, const std::string& gamePath);
    void WritePortVersion();
private:
    fs::path mGamePath;
    std::vector<uint8_t> mGameData;
    std::string mSearchPath;
};
