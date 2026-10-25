#pragma once

#include <stdbool.h>

#ifdef __cplusplus
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "port/ModRules.h"

namespace Ship {
class Archive;
class ArchiveManager;
} // namespace Ship

// A romhack is an o2r in mods/~romhacks holding only what the hack changed. It is extracted from the patched ROM
// through a curated config.yml entry, loads on top of sm64.o2r and under the regular mods, and only one is active
// at a time. See docs/romhacks/DESIGN.md.
namespace Romhack {

// Folders starting with ~ are managed by the port.
constexpr const char* kFolder = "~romhacks";
constexpr const char* kManifestFile = "manifest.json";

struct Info {
    std::string id; // the config.yml romhack.id, picks the code port in specific/<Id>.cpp
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string website;
    std::string romSha1;
    // The features the hack turns off or pins (disable / force in its recipe).
    ModRules::Rules rules;
};

// A hack config.yml has a recipe for, whether or not it has been extracted.
struct Supported {
    std::string id;
    std::string name;
    std::string version;
    std::string website;
};
// Read from config.yml. Empty on builds without Torch.
std::vector<Supported> ListSupported();

// The manifest.json written into a romhack o2r, and the Info read back from one.
std::string ManifestText(const Info& info);
std::optional<Info> ParseManifest(const std::string& text);

// Absolute mods/~romhacks directory.
std::string Directory();
// Stems of every romhack o2r in mods/~romhacks.
std::vector<std::string> ListArchives();

// Strips -hack/--hack <stem> from argv; the stem wins over the saved selection for this launch.
std::vector<std::string> ConsumeLaunchArgs(int argc, char* argv[]);

std::string GetSelected();
// Saves the selection; it takes effect on the next start.
void SetSelected(const std::string& stem);

// Adds the selected romhack archive. Call once, right after sm64.o2r is mounted and before the mods.
void Mount(const std::shared_ptr<Ship::ArchiveManager>& archives);
// Runs the active port's ApplyDataTables. Must run before ShipInit so enhancements see the hack's values.
void ApplyDataTables();

const Info* Active();
const std::string& ActiveStem();
// The mounted romhack archive, so the Mods menu can leave it out.
const std::shared_ptr<Ship::Archive>& ActiveArchive();

} // namespace Romhack

extern "C" {
#endif

bool Romhack_IsActive(void);
// The active hack's name, or NULL on vanilla.
const char* Romhack_GetName(void);
// The active hack's version (e.g. "1.2"), or NULL when unknown or on vanilla.
const char* Romhack_GetVersion(void);
// The active hack's identifier (its config.yml romhack.id), or NULL on vanilla.
const char* Romhack_GetId(void);

#ifdef __cplusplus
}
#endif
