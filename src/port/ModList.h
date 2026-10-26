#pragma once

#include <memory>
#include <string>
#include <vector>

#include "port/ModRules.h"

namespace Ship {
class Archive;
}

// Every mod archive with a manifest found at startup, enabled or not, with the settings the Mods menu changes.
namespace ModList {

struct Entry {
    std::shared_ptr<Ship::Archive> archive;
    bool enabled;
    int priority;
    std::string status;
    bool failed = false;
    ModRules::Rules rules;
    // Enabled and runs on the active base, so its files and rules apply.
    bool mounted = false;
    // What the mod brings besides files: C code or libraries (main, binaries), and post-processing passes or
    // material shaders (shaders.passes, shaders.materials).
    bool code = false;
    bool shaders = false;
};

// Remembers the mounted mods, unmounts the disabled ones and those made for another base (works_with), and orders
// the rest by priority. Mods with a higher priority are mounted later, so their files win, and their code
// initializes later.
void Apply();
std::vector<Entry>& Entries();

bool IsEnabled(const std::string& name);
void SetEnabled(const std::string& name, bool enabled);
int Priority(const std::string& name);
void SetPriority(const std::string& name, int priority);
// Saves the Mods menu's lists: enabled mods from the one that wins (highest priority) down, and the disabled ones.
void SetOrder(const std::vector<std::string>& enabledFirstWins, const std::vector<std::string>& disabled);

void ClearStatus();
void SetStatus(const std::string& name, const std::string& status, bool failed);

} // namespace ModList
