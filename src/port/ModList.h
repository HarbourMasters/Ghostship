#pragma once

#include <memory>
#include <string>
#include <vector>

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
};

// Remembers the mounted mods, unmounts the disabled ones and orders the rest by priority. Mods with a higher
// priority are mounted later, so their files win, and their code initializes later.
void Apply();
std::vector<Entry>& Entries();

bool IsEnabled(const std::string& name);
void SetEnabled(const std::string& name, bool enabled);
int Priority(const std::string& name);
void SetPriority(const std::string& name, int priority);

void ClearStatus();
void SetStatus(const std::string& name, const std::string& status, bool failed);

} // namespace ModList
