#include "Romhack.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <libultraship/bridge/consolevariablebridge.h>
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <ship/resource/File.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/resource/archive/O2rArchive.h>
#include <spdlog/spdlog.h>

#if __has_include(<yaml-cpp/yaml.h>)
#include <yaml-cpp/yaml.h>
#define ROMHACK_HAS_CONFIG 1
#endif

#include "port/ui/cvar_prefixes.h"

#define CVAR_ROMHACK_SELECTED CVAR_SETTING("Romhack.Selected")

namespace fs = std::filesystem;

namespace Romhack {

static std::optional<std::string> sLaunchStem;
static std::optional<Info> sActive;
static std::string sActiveStem;
static std::shared_ptr<Ship::Archive> sActiveArchive;

std::string RecipeDirectory() {
    return Ship::Context::GetPathRelativeToAppDirectory("romhacks");
}

std::vector<std::string> ListRecipes() {
    std::vector<std::string> packages;
    std::vector<std::string> names;
    const std::string shipped = (fs::path(Ship::Context::GetAppBundlePath()) / "romhacks").generic_string();
    for (const std::string& dir : { RecipeDirectory(), shipped }) {
        std::vector<fs::path> found;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".o2r") {
                found.push_back(entry.path());
            }
        }
        std::sort(found.begin(), found.end());
        for (const fs::path& path : found) {
            const std::string name = path.filename().string();
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(name);
                packages.push_back(path.generic_string());
            }
        }
    }
    return packages;
}

static std::shared_ptr<Ship::O2rArchive> OpenRecipe(const std::string& package) {
    auto archive = std::make_shared<Ship::O2rArchive>(package);
    archive->Load();
    if (!archive->IsInitialized()) {
        SPDLOG_WARN("Romhack: could not open recipe package {}", package);
        return nullptr;
    }
    return archive;
}

static std::optional<std::string> ReadText(const std::shared_ptr<Ship::Archive>& archive, const std::string& path) {
    auto file = archive->LoadFile(path);
    if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr) {
        return std::nullopt;
    }
    return std::string(file->Buffer->begin() + file->BufferOffset, file->Buffer->end());
}

std::optional<std::string> ReadRecipeConfig(const std::string& package) {
    auto archive = OpenRecipe(package);
    return archive ? ReadText(archive, "config.yml") : std::nullopt;
}

std::optional<std::string> UnpackRecipe(const std::string& package) {
    static std::unordered_map<std::string, std::string> sUnpacked;
    if (auto it = sUnpacked.find(package); it != sUnpacked.end()) {
        return it->second;
    }
    auto archive = OpenRecipe(package);
    if (archive == nullptr) {
        return std::nullopt;
    }
    const fs::path dir =
        Ship::Context::GetPathRelativeToAppDirectory("cache/romhacks/" + fs::path(package).stem().string());
    std::error_code ec;
    fs::remove_all(dir, ec);
    for (const auto& [hash, path] : *archive->ListFiles()) {
        auto file = archive->LoadFile(path);
        if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr) {
            SPDLOG_WARN("Romhack: {} in {} didn't load", path, package);
            return std::nullopt;
        }
        const fs::path out = dir / path;
        fs::create_directories(out.parent_path(), ec);
        std::ofstream stream(out, std::ios::binary);
        stream.write(file->Buffer->data() + file->BufferOffset, file->Buffer->size() - file->BufferOffset);
    }
    SPDLOG_INFO("Romhack: unpacked recipe {} to {}", package, dir.generic_string());
    sUnpacked[package] = dir.generic_string();
    return dir.generic_string();
}

static void AddSupported(const YAML::Node& config, std::vector<Supported>& hacks, const std::string& package);

std::vector<Supported> ListSupported() {
    std::vector<Supported> hacks;
#ifdef ROMHACK_HAS_CONFIG
    const std::string configPath = (fs::path(Ship::Context::GetAppBundlePath()) / "config.yml").generic_string();
    try {
        AddSupported(YAML::LoadFile(configPath), hacks, "");
    } catch (const YAML::Exception& e) { SPDLOG_WARN("Romhack: could not read {}: {}", configPath, e.what()); }
    for (const std::string& package : ListRecipes()) {
        try {
            if (auto text = ReadRecipeConfig(package)) {
                AddSupported(YAML::Load(*text), hacks, package);
            }
        } catch (const YAML::Exception& e) { SPDLOG_WARN("Romhack: could not read {}: {}", package, e.what()); }
    }
#endif
    std::sort(hacks.begin(), hacks.end(), [](const Supported& a, const Supported& b) { return a.name < b.name; });
    return hacks;
}

static void AddSupported(const YAML::Node& config, std::vector<Supported>& hacks, const std::string& package) {
#ifdef ROMHACK_HAS_CONFIG
    std::vector<std::string> buildable;
    for (const auto& entry : config) {
        const YAML::Node rom = entry.second;
        if (rom.IsMap() && rom["preprocess"]) {
            for (const auto& job : rom["preprocess"]) {
                if (job.second["target"]) {
                    buildable.push_back(job.second["target"].as<std::string>());
                }
            }
        }
    }
    for (const auto& entry : config) {
        const YAML::Node rom = entry.second;
        if (!rom.IsMap() || !rom["romhack"]) {
            continue;
        }
        const YAML::Node hack = rom["romhack"];
        Supported info;
        info.id = hack["id"].as<std::string>("");
        info.name = rom["name"].as<std::string>(info.id);
        info.version = hack["version"].as<std::string>("");
        info.website = hack["website"].as<std::string>("");
        info.package = package;
        info.fromBaseRom = !package.empty() &&
                           std::find(buildable.begin(), buildable.end(), entry.first.as<std::string>()) != buildable.end();
        if (std::none_of(hacks.begin(), hacks.end(), [&](const Supported& s) { return s.id == info.id; })) {
            hacks.push_back(std::move(info));
        }
    }
#endif
}

// The romhack block is ours; the other keys are the ones LUS reads into ArchiveManifest. No "main", so the
// script loader never tries to build it.
std::string ManifestText(const Info& info) {
    nlohmann::ordered_json json;
    json["name"] = info.name;
    json["version"] = info.version;
    json["author"] = info.author;
    json["description"] = info.description;
    json["website"] = info.website;
    json["romhack"] = { { "id", info.id }, { "rom_sha1", info.romSha1 } };
    ModRules::Write(info.rules, json);
    if (!info.codeMain.empty()) {
        json["main"] = info.codeMain;
        json["code_version"] = info.codeVersion;
        json["api_version"] = info.apiVersion;
    }
    return json.dump(4);
}

std::optional<Info> ParseManifest(const std::string& text) {
    try {
        const auto json = nlohmann::json::parse(text);
        if (!json.contains("romhack") || !json["romhack"].is_object()) {
            return std::nullopt;
        }
        Info info;
        info.id = json["romhack"].value("id", "");
        info.romSha1 = json["romhack"].value("rom_sha1", "");
        info.name = json.value("name", info.id);
        info.version = json.value("version", "");
        info.author = json.value("author", "");
        info.description = json.value("description", "");
        info.website = json.value("website", "");
        info.rules = ModRules::Parse(json);
        return info;
    } catch (const std::exception& e) {
        SPDLOG_WARN("Romhack: unreadable {}: {}", kManifestFile, e.what());
        return std::nullopt;
    }
}

std::string Directory() {
    return Ship::Context::GetPathRelativeToAppDirectory(std::string("mods/") + kFolder);
}

std::vector<std::string> ListArchives() {
    std::vector<std::string> stems;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(Directory(), ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".o2r") {
            stems.push_back(entry.path().stem().string());
        }
    }
    std::sort(stems.begin(), stems.end());
    return stems;
}

std::vector<std::string> ConsumeLaunchArgs(int argc, char* argv[]) {
    std::vector<std::string> rest;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if ((arg == "-hack" || arg == "--hack") && i + 1 < argc) {
            sLaunchStem = argv[++i];
            continue;
        }
        rest.push_back(arg);
    }
    return rest;
}

std::string GetSelected() {
    if (sLaunchStem.has_value()) {
        return *sLaunchStem;
    }
    return CVarGetString(CVAR_ROMHACK_SELECTED, "");
}

void SetSelected(const std::string& stem) {
    sLaunchStem.reset();
    CVarSetString(CVAR_ROMHACK_SELECTED, stem.c_str());
    CVarSave();
}

void Mount(const std::shared_ptr<Ship::ArchiveManager>& archives) {
    const std::string stem = GetSelected();
    if (stem.empty()) {
        return;
    }

    const fs::path path = fs::path(Directory()) / (stem + ".o2r");
    if (!fs::exists(path)) {
        SPDLOG_WARN("Romhack: selected archive {} is missing, starting vanilla", path.generic_string());
        return;
    }

    SPDLOG_INFO("Romhack: loading {}", path.generic_string());
    auto archive = archives->AddArchive(path.generic_string());
    if (archive == nullptr) {
        SPDLOG_WARN("Romhack: could not open {}, starting vanilla", path.generic_string());
        return;
    }

    std::optional<Info> info;
    auto file = archive->LoadFile(kManifestFile);
    if (file != nullptr && file->IsLoaded && file->Buffer != nullptr) {
        info = ParseManifest(std::string(file->Buffer->begin() + file->BufferOffset, file->Buffer->end()));
    }
    if (!info.has_value()) {
        SPDLOG_WARN("Romhack: {} has no romhack manifest; treating it as an asset-only hack", stem);
        info = Info{ stem, stem };
    }

    sActive = info;
    sActiveStem = stem;
    sActiveArchive = archive;
}


const Info* Active() {
    return sActive.has_value() ? &*sActive : nullptr;
}

const std::string& ActiveStem() {
    return sActiveStem;
}

const std::shared_ptr<Ship::Archive>& ActiveArchive() {
    return sActiveArchive;
}

} // namespace Romhack

extern "C" bool Romhack_IsActive(void) {
    return Romhack::Active() != nullptr;
}

extern "C" const char* Romhack_GetName(void) {
    const auto* info = Romhack::Active();
    return info != nullptr ? info->name.c_str() : nullptr;
}

extern "C" const char* Romhack_GetVersion(void) {
    const auto* info = Romhack::Active();
    return info != nullptr && !info->version.empty() ? info->version.c_str() : nullptr;
}

extern "C" const char* Romhack_GetId(void) {
    const auto* info = Romhack::Active();
    return info != nullptr && !info->id.empty() ? info->id.c_str() : nullptr;
}
