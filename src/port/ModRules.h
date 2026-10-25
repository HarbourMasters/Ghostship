#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace Ship {
class Archive;
}

// The keys of manifest.json Ghostship reads on top of LUS's own (LUS ignores keys it doesn't know):
//   "works_with": ["vanilla", { "name": "Star Road", "version": ">=8" }]  bases the mod runs on; missing = any
//   "disable":    ["cheats.*", "enhancements.select_all_stars"]           features the mod turns off
//   "force":      { "graphics.interpolation_fps": 30 }                    features the mod pins to a value
// A romhack's rules come from its config.yml recipe and are written into its manifest at extraction.
namespace ModRules {

constexpr const char* kBaseVanilla = "vanilla";

struct BaseRef {
    std::string name;     // "vanilla", a romhack's name, or "*"
    std::string versions; // a range in the LUS dependency syntax, empty for any
};

struct Rules {
    std::optional<std::vector<BaseRef>> worksWith;
    std::vector<std::string> disable;
    std::vector<std::pair<std::string, nlohmann::json>> force;
    bool isRomhack = false; // the manifest has a romhack block
};

Rules Parse(const nlohmann::json& manifest);
// Adds the rules' keys to a manifest being written.
void Write(const Rules& rules, nlohmann::ordered_json& manifest);
// The archive's own manifest.json, read from it rather than the merged file system, where every archive's
// manifest.json shadows the others.
std::optional<nlohmann::json> ReadManifest(Ship::Archive& archive);

// version against a list of comparisons (>=, >, <=, <, =) separated by spaces or commas; a bare version is a minimum.
bool VersionInRange(const std::string& version, const std::string& range);
// Why a mod can't run on the active base (vanilla or the active romhack), or nullopt when it can.
std::optional<std::string> IncompatibleReason(const Rules& rules);

// Hands every rule source to the feature registry in load order: the built-in rules, the romhack, then each mounted
// mod. Call after the mods are mounted and ordered, before ShipInit.
void Resolve();

} // namespace ModRules
