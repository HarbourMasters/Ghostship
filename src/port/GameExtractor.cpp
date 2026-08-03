#ifdef _WIN32
#include <Windows.h>
#include <winuser.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif
#include "port/build.h"
#include "GameExtractor.h"
#include <cstdio>
#include <unordered_map>
#include <fstream>
#include <algorithm>

#include "ship/Context.h"
#include "spdlog/spdlog.h"
#include <port/Engine.h>

#ifdef unix
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// Defined unconditionally (declared in the header for all platforms).
std::string GameExtractor::sLastError;

#ifndef __SWITCH__
#include "Companion.h"

#ifdef __EMSCRIPTEN__
#include "port/web/WebUtils.h"
#elif !defined(__IOS__) && !defined(__ANDROID__) && !defined(__SWITCH__)
#include "portable-file-dialogs.h"
#endif

std::unordered_map<std::string, std::string> mGameList = {
    { "8a20a5c83d6ceb0f0506cfc9fa20d8f438cafe51", "Super Mario 64 (JP)" },
    { "9bef1128717f958171a4afac3ed78ee2bb4e86ce", "Super Mario 64 (US)" },
};

static std::filesystem::path Utf8ToFsPath(const std::string& utf8) {
    return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8.c_str()));
}

bool GameExtractor::RunStandalone(std::string rom) {
    // Store both path and already-read data
    std::string romPath;
    std::vector<uint8_t> romData;

    if (!std::filesystem::exists(rom)) {
        return false;
    }

    std::ifstream inFile(rom, std::ios::binary);
    if (!inFile.is_open()) {
        SPDLOG_INFO("Failed to open ROM at path: {}, continuing", rom);
        return false;
    }

    inFile.seekg(0, std::ios::end);
    size_t fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    if (!inFile.read(reinterpret_cast<char*>(data.data()), fileSize)) {
        SPDLOG_INFO("Failed to read ROM at path: {}, continuing", rom);
        return false;
    }

    inFile.close();
    std::string hash = Companion::CalculateHash(data);

    if (mGameList.find(hash) != mGameList.end()) {
        romPath = rom;
        romData = std::move(data);
    }

    // Load file if it is not already open
    if (romData.empty()) {
        std::ifstream inFile(romPath, std::ios::binary);
        if (!inFile.is_open()) {
            return false;
        }

        romData = std::vector<uint8_t>(std::istreambuf_iterator<char>(inFile), {});
        inFile.close();
    }

    this->mGamePath = romPath;
    this->mGameData = std::move(romData);

    return true;
}

bool GameExtractor::SelectGameFromUI() {
    //// Store both path and already-read data
    std::string romPath;
    std::vector<uint8_t> romData;

#if defined(__EMSCRIPTEN__)
    romPath = WebFilePicker_PickROM();
    if (romPath.empty()) {
        return false;
    }
#elif !defined(__IOS__) && !defined(__ANDROID__) && !defined(__SWITCH__)
    // Desktop: fallback to file dialogue if no baserom found
    // if (!foundGame) {
    if (!pfd::settings::available()) {
        SPDLOG_ERROR("portable-file-dialogs is not available on this system.");
        return false;
    }

    auto selection = pfd::open_file("Select a file", ".", { "N64 Roms", "*.z64" }).result();
    if (selection.empty()) {
        return false;
    }

    romPath = selection[0];
    //}
#else
    // Mobile: fallback to baserom.us.z64
    if (/*!foundGame && */ !std::filesystem::exists(Ship::Context::GetPathRelativeToAppDirectory("baserom.us.z64"))) {
        SPDLOG_ERROR("baserom not found");
        return false;
    }

    // if (!foundGame) {
    romPath = Ship::Context::GetPathRelativeToAppDirectory("baserom.us.z64");
    //}
#endif

    // Load file if it is not already open
    if (romData.empty()) {
        const std::filesystem::path romFsPath = Utf8ToFsPath(romPath);
        if (!std::filesystem::exists(romFsPath)) {
            SPDLOG_ERROR("Failed to find ROM at path: {}", romPath);
            return false;
        }

        std::ifstream inFile(romFsPath, std::ios::binary);
        if (!inFile.is_open()) {
            return false;
        }

        romData = std::vector<uint8_t>(std::istreambuf_iterator<char>(inFile), {});
        inFile.close();
    }

    this->mGamePath = romPath;
    this->mGameData = std::move(romData);

    return true;
}

void GameExtractor::SetSearchPath(const std::string& path) {
    mSearchPath = path;
}

void GameExtractor::GetRoms(std::vector<std::string>& roms) {
#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    std::string search = std::string(mSearchPath + "\\*");
    HANDLE h = FindFirstFileA(search.c_str(), &ffd);

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char* ext = PathFindExtensionA(ffd.cFileName);

            // Check for any standard N64 rom file extensions.
            if (ext != NULL &&
                (strcmp(ext, ".z64") == 0) /* || (strcmp(ext, ".n64") == 0) || (strcmp(ext, ".v64") == 0)*/)
                roms.push_back(ffd.cFileName);
        }
    } while (FindNextFileA(h, &ffd) != 0);
    // if (h != nullptr) {
    //    CloseHandle(h);
    //}
#elif unix
    // Open the directory of the app.
    DIR* d = opendir(mSearchPath.c_str());
    struct dirent* dir;

    if (d != NULL) {
        // Go through each file in the directory
        while ((dir = readdir(d)) != NULL) {
            struct stat path;

            // Check if current entry is not folder
            stat(dir->d_name, &path);
            if (S_ISREG(path.st_mode)) {

                // Get the position of the extension character.
                char* ext = strrchr(dir->d_name, '.');
                if (ext != NULL &&
                    (strcmp(ext, ".z64") == 0 /* || strcmp(ext, ".n64") == 0 || strcmp(ext, ".v64") == 0*/)) {
                    roms.push_back(dir->d_name);
                }
            }
        }
    }
    closedir(d);
#else
    for (const auto& file : std::filesystem::directory_iterator(mSearchPath)) {
        if (file.is_directory()) {
            continue;
        }
        if (/*(file.path().extension() == ".n64") || */(file.path().extension() == ".z64")/* ||
            (file.path().extension() == ".v64")*/) {
            roms.push_back((file.path().generic_string()));
        }
    }
#endif
}

std::optional<std::string> GameExtractor::ValidateChecksum() const {
    const auto rom = new N64::Cartridge(this->mGameData);
    rom->Initialize();
    auto hash = rom->GetHash();

    if (mGameList.find(hash) == mGameList.end()) {
        return std::nullopt;
    }

    return mGameList[hash];
}

std::optional<std::string> GameExtractor::DetectVersion(const std::string& romPath) {
    std::error_code ec;
    if (!std::filesystem::exists(Utf8ToFsPath(romPath), ec)) {
        return std::nullopt;
    }
    std::ifstream in(Utf8ToFsPath(romPath), std::ios::binary);
    if (!in.is_open()) {
        return std::nullopt;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    in.close();
    if (data.empty()) {
        return std::nullopt;
    }
    const std::string hash = Companion::CalculateHash(data);
    const auto it = mGameList.find(hash);
    if (it == mGameList.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::pair<std::string, std::string>>
GameExtractor::FindSupportedRoms(const std::vector<std::string>& searchPaths) {
    std::vector<std::pair<std::string, std::string>> out;
    std::vector<std::string> seenVersions;
    for (const auto& dir : searchPaths) {
        std::error_code ec;
        if (dir.empty() || !std::filesystem::is_directory(Utf8ToFsPath(dir), ec)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(Utf8ToFsPath(dir), ec)) {
            if (entry.is_directory() || entry.path().extension() != ".z64") {
                continue;
            }
            const std::string full = entry.path().generic_string();
            const auto version = DetectVersion(full);
            if (!version.has_value()) {
                continue; // Only offer recognized ROMs.
            }
            // One entry per version so the user isn't shown duplicate copies of the same ROM.
            if (std::find(seenVersions.begin(), seenVersions.end(), *version) != seenVersions.end()) {
                continue;
            }
            seenVersions.push_back(*version);
            out.emplace_back(full, *version);
        }
    }
    return out;
}

void GameExtractor::WritePortVersion() {
    auto writer = LUS::BinaryWriter();
    writer.SetEndianness(Torch::Endianness::Big);
    writer.Write((uint16_t)gBuildVersionMajor);
    writer.Write((uint16_t)gBuildVersionMinor);
    writer.Write((uint16_t)gBuildVersionPatch);
    writer.Close();

    Companion::Instance->RegisterCompanionFile("portVersion", writer.ToVector());
}

std::string GameExtractor::GetRomPath() {
    return mGamePath.generic_string();
}

bool GameExtractor::Parse(std::atomic<size_t>& totalAssets, std::string appShortName) {
    const std::string assets_path = Ship::Context::GetAppBundlePath();
    const std::string game_path = Ship::Context::GetAppDirectoryPath(appShortName);

    sLastError.clear();
    Companion::Instance = new Companion(this->mGameData, ArchiveType::O2R, false, assets_path, game_path);
    Companion::Instance->SetProcess(false);
    try {
        Companion::Instance->Init(ExportType::Binary, totalAssets, false);
#ifdef __EMSCRIPTEN__
        Companion::Instance->Process(totalAssets);
#endif
    } catch (const std::exception& e) {
        sLastError = e.what();
        SPDLOG_ERROR("Failed to process O2R: {}", e.what());
        return false;
    }

    return true;
}

bool GameExtractor::GenerateOTR(std::string appShortName) {
    std::atomic<size_t> assetCount{ 0 };
    return GenerateOTR(assetCount, appShortName);
}

bool GameExtractor::GenerateOTR(std::atomic<size_t>& assetCount, std::string appShortName) {
    return GenerateOTRTo(assetCount, Ship::Context::GetAppBundlePath(),
                         Ship::Context::GetAppDirectoryPath(appShortName));
}

bool GameExtractor::GenerateOTRTo(std::atomic<size_t>& assetCount, const std::string& assetsPath,
                                  const std::string& gamePath) {
    Companion::Instance = new Companion(this->mGameData, ArchiveType::O2R, false, assetsPath, gamePath);
    this->WritePortVersion();
    try {
        Companion::Instance->Init(ExportType::Binary, assetCount, true);
    } catch (const std::exception& e) {
        SPDLOG_INFO("Failed to process O2R {}", e.what());
        return false;
    }

    return true;
}
#else
static bool GameExtractor::GenAssetFile() {
    return false;
}

std::optional<std::string> GameExtractor::ValidateChecksum() const {
    return std::nullopt;
}

bool GameExtractor::SelectGameFromUI() {
    return false;
}

void GameExtractor::GetRoms(std::vector<std::string>& roms) {
    // None
}

std::optional<std::string> GameExtractor::DetectVersion(const std::string& romPath) {
    return std::nullopt;
}

std::vector<std::pair<std::string, std::string>>
GameExtractor::FindSupportedRoms(const std::vector<std::string>& searchPaths) {
    return {};
}

bool GameExtractor::GenerateOTR(std::string appShortName) {
    return false;
}

bool GameExtractor::GenerateOTR(std::atomic<size_t>& assetCount, std::string appShortName) {
    return false;
}

bool GameExtractor::GenerateOTRTo(std::atomic<size_t>& assetCount, const std::string& assetsPath,
                                  const std::string& gamePath) {
    return false;
}

void GameExtractor::WritePortVersion() {
    // None
}
#endif