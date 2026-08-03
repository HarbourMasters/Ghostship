#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>
#include <optional>
#include <atomic>
#include <string>
#include <utility>

namespace fs = std::filesystem;

class GameExtractor {
public:
    // Message from the most recent failed extraction (empty on success). Surfaced
    // in the extraction-failure popup so the user sees the real cause.
    static std::string sLastError;

    static bool GenAssetFile();
    std::optional<std::string> ValidateChecksum() const;
    bool RunStandalone(std::string rom);
    bool SelectGameFromUI();
    void SetSearchPath(const std::string& path);
    void GetRoms(std::vector<std::string>& roms);
    // Returns the human-readable version name for a ROM file (e.g. "Super Mario 64 (US)"),
    // or nullopt if the file isn't a recognized/supported ROM.
    static std::optional<std::string> DetectVersion(const std::string& romPath);
    // Scans the given directories for supported .z64 ROMs, returning (full path, version
    // name) pairs deduplicated by version so the user is offered one entry per version.
    std::vector<std::pair<std::string, std::string>> FindSupportedRoms(const std::vector<std::string>& searchPaths);
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
