#if defined(_WIN32) && !defined(__WINRT__)
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
#include <yaml-cpp/yaml.h>

#ifdef __EMSCRIPTEN__
#include "port/web/WebUtils.h"
#elif !defined(__IOS__) && !defined(__ANDROID__) && !defined(__SWITCH__) && !defined(__WINRT__)
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

bool GameExtractor::LoadRom(const std::string& rom) {
    std::ifstream in(Utf8ToFsPath(rom), std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    this->mGameData = std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
    this->mGamePath = rom;
    return !this->mGameData.empty();
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
#elif !defined(__IOS__) && !defined(__ANDROID__) && !defined(__SWITCH__) && !defined(__WINRT__)
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
#if defined(_WIN32) && !defined(__WINRT__)
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

static std::optional<RomhackRecipe> RomhackRecipeFor(const std::vector<uint8_t>& romData, const std::string& configDir,
                                                     const std::string& requested);

// Each pass gets its own Torch instance; the previous one holds the ROM and what that pass kept, so free it.
static void ReplaceCompanion(Companion* next) {
    Companion* previous = Companion::Instance;
    Companion::Instance = next;
    delete previous;
}

void GameExtractor::ReleaseTorch() {
    ReplaceCompanion(nullptr);
}

bool GameExtractor::Parse(std::atomic<size_t>& totalAssets, std::string appShortName) {
    std::string assets_path = Ship::Context::GetAppBundlePath();
    if (const auto romhack = RomhackRecipeFor(this->mGameData, assets_path, mRequestedPackage);
        romhack && !romhack->sourceDir.empty()) {
        assets_path = romhack->sourceDir;
    }
    const std::string game_path = Ship::Context::GetAppDirectoryPath(appShortName);

    sLastError.clear();
    ReplaceCompanion(new Companion(this->mGameData, ArchiveType::O2R, false, assets_path, game_path));
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

// A recipe's force values as JSON: booleans and numbers keep their type, so "force: { x: true }" pins x on.
static nlohmann::json YamlToJson(const YAML::Node& node) {
    if (node.IsSequence()) {
        nlohmann::json out = nlohmann::json::array();
        for (const auto& item : node) {
            out.push_back(YamlToJson(item));
        }
        return out;
    }
    if (node.IsMap()) {
        nlohmann::json out = nlohmann::json::object();
        for (const auto& item : node) {
            out[item.first.as<std::string>()] = YamlToJson(item.second);
        }
        return out;
    }
    if (!node.IsScalar()) {
        return nullptr;
    }
    const std::string text = node.Scalar();
    if (node.Tag() != "!") {
        if (text == "true" || text == "false") {
            return text == "true";
        }
        try {
            size_t used = 0;
            const long long i = std::stoll(text, &used, 0);
            if (used == text.size()) {
                return i;
            }
            const double d = std::stod(text, &used);
            if (used == text.size()) {
                return d;
            }
        } catch (const std::exception&) {}
    }
    return text;
}

static void PackRomhackFiles(const std::filesystem::path& folder) {
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) {
        SPDLOG_WARN("Romhack files folder {} doesn't exist", folder.generic_string());
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::filesystem::path rel = entry.path().lexically_relative(folder);
        rel.replace_extension();
        std::ifstream in(entry.path(), std::ios::binary);
        std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        Companion::Instance->RegisterCompanionFile(rel.generic_string(), std::move(data));
    }
}

static void PackRomhackCode(const std::filesystem::path& folder, Romhack::Info& info) {
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) {
        SPDLOG_WARN("Romhack code folder {} doesn't exist", folder.generic_string());
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::filesystem::path rel = entry.path().lexically_relative(folder);
        std::ifstream in(entry.path(), std::ios::binary);
        std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (rel == "manifest.json") {
            try {
                const auto json = nlohmann::json::parse(data.begin(), data.end());
                info.codeMain = json.value("main", "");
                info.codeVersion = json.value("code_version", 0);
                info.apiVersion = json.value("api_version", 0);
            } catch (const std::exception& e) {
                SPDLOG_WARN("Romhack code {}: {}", entry.path().generic_string(), e.what());
            }
            continue;
        }
        Companion::Instance->RegisterCompanionFile(rel.generic_string(), std::move(data));
    }
}

static std::optional<RomhackRecipe> RecipeFromConfig(const YAML::Node& config, const std::string& hash,
                                                     const std::string& configPath) {
    try {
        const YAML::Node rom = config[hash];
        if (!rom.IsMap() || !rom["romhack"]) {
            return std::nullopt;
        }
        const YAML::Node hack = rom["romhack"];
        RomhackRecipe recipe;
        recipe.info.id = hack["id"].as<std::string>("");
        recipe.info.name = rom["name"].as<std::string>(recipe.info.id);
        recipe.info.version = hack["version"].as<std::string>("");
        recipe.info.author = hack["author"].as<std::string>("");
        recipe.info.description = hack["description"].as<std::string>("");
        recipe.info.website = hack["website"].as<std::string>("");
        recipe.info.romSha1 = hash;
        // disable / force in the recipe use the manifest's own keys.
        nlohmann::json rules = nlohmann::json::object();
        for (const char* key : { "disable", "force" }) {
            if (hack[key]) {
                rules[key] = YamlToJson(hack[key]);
            }
        }
        recipe.info.rules = ModRules::Parse(rules);
        recipe.output = rom["config"]["output"]["binary"].as<std::string>("");
        recipe.files = hack["files"].as<std::string>("");
        recipe.code = hack["code"].as<std::string>("");
        if (recipe.info.id.empty() || recipe.output.empty()) {
            SPDLOG_WARN("Romhack entry {} in {} needs romhack.id and config.output.binary", hash, configPath);
            return std::nullopt;
        }
        return recipe;
    } catch (const YAML::Exception& e) { SPDLOG_WARN("Could not read {}: {}", configPath, e.what()); }
    return std::nullopt;
}

static std::optional<RomhackRecipe> RecipeFromBaseRom(const YAML::Node& config, const std::string& hash,
                                                      const std::string& package) {
    const YAML::Node base = config[hash];
    if (!base.IsMap() || !base["preprocess"]) {
        return std::nullopt;
    }
    std::string target;
    for (const auto& job : base["preprocess"]) {
        if (job.second["target"]) {
            target = job.second["target"].as<std::string>();
        }
    }
    return target.empty() ? std::nullopt : RecipeFromConfig(config, target, package);
}

static std::optional<RomhackRecipe> RomhackRecipeFor(const std::vector<uint8_t>& romData, const std::string& configDir,
                                                     const std::string& requested) {
    if (romData.empty()) {
        return std::nullopt;
    }
    const std::string hash = Companion::CalculateHash(romData);
    const std::string configPath = (std::filesystem::path(configDir) / "config.yml").generic_string();
    if (requested.empty()) {
        try {
            if (auto recipe = RecipeFromConfig(YAML::LoadFile(configPath), hash, configPath)) {
                return recipe;
            }
        } catch (const YAML::Exception& e) { SPDLOG_WARN("Could not read {}: {}", configPath, e.what()); }
    }

    for (const std::string& package : Romhack::ListRecipes()) {
        if (!requested.empty() && package != requested) {
            continue;
        }
        const auto text = Romhack::ReadRecipeConfig(package);
        if (!text.has_value()) {
            continue;
        }
        std::optional<RomhackRecipe> recipe;
        try {
            const YAML::Node config = YAML::Load(*text);
            recipe = RecipeFromConfig(config, hash, package);
            if (!recipe.has_value() && !requested.empty()) {
                recipe = RecipeFromBaseRom(config, hash, package);
            }
        } catch (const YAML::Exception& e) { SPDLOG_WARN("Could not read {}: {}", package, e.what()); }
        if (!recipe.has_value()) {
            continue;
        }
        const auto dir = Romhack::UnpackRecipe(package);
        if (!dir.has_value()) {
            return std::nullopt;
        }
        recipe->sourceDir = *dir;
        return recipe;
    }
    return std::nullopt;
}

std::optional<RomhackRecipe> GameExtractor::GetRomhack() const {
    return RomhackRecipeFor(this->mGameData, Ship::Context::GetAppBundlePath(), mRequestedPackage);
}

bool GameExtractor::GenerateOTRTo(std::atomic<size_t>& assetCount, const std::string& assetsPath,
                                  const std::string& gamePath) {
    const auto romhack = RomhackRecipeFor(this->mGameData, assetsPath, mRequestedPackage);
    std::string sourcePath = assetsPath;
    if (romhack.has_value()) {
        std::error_code ec;
        std::filesystem::create_directories((std::filesystem::path(gamePath) / romhack->output).parent_path(), ec);
        if (!romhack->sourceDir.empty()) {
            sourcePath = romhack->sourceDir;
        }
    }

    ReplaceCompanion(new Companion(this->mGameData, ArchiveType::O2R, false, sourcePath, gamePath));
    this->WritePortVersion();
    if (romhack.has_value()) {
        Romhack::Info info = romhack->info;
        if (!romhack->files.empty()) {
            PackRomhackFiles(std::filesystem::path(sourcePath) / romhack->files);
        }
        if (!romhack->code.empty()) {
            PackRomhackCode(std::filesystem::path(sourcePath) / romhack->code, info);
        }
        // Stamps the hack's identity into its o2r, so the runtime never needs config.yml to know what it is.
        const std::string manifest = Romhack::ManifestText(info);
        Companion::Instance->RegisterCompanionFile(Romhack::kManifestFile,
                                                   std::vector<char>(manifest.begin(), manifest.end()));
    }
    try {
        Companion::Instance->Init(ExportType::Binary, assetCount, true);
#ifdef __EMSCRIPTEN__
        // Torch's Init() skips Process() on the web build, so run the export pass here like Parse() does.
        Companion::Instance->Process(assetCount);
#endif
    } catch (const std::exception& e) {
        sLastError = e.what();
        SPDLOG_ERROR("Failed to process O2R: {}", e.what());
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

std::optional<RomhackRecipe> GameExtractor::GetRomhack() const {
    return std::nullopt;
}

bool GameExtractor::LoadRom(const std::string& rom) {
    return false;
}

bool GameExtractor::SelectGameFromUI() {
    return false;
}

void GameExtractor::ReleaseTorch() {
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