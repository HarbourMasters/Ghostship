#include "port/ModList.h"
#include "port/ShipCompat.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>

#include <algorithm>
#include <cctype>

namespace {

std::vector<ModList::Entry> sEntries;

std::string Key(const char* setting, const std::string& name) {
    std::string key = std::string("gModList.") + setting + ".";
    for (const char c : name) {
        key += std::isalnum((unsigned char)c) ? c : '_';
    }
    return key;
}

} // namespace

void ModList::Apply() {
    auto manager = ShipCompat::GetResourceManager()->GetArchiveManager();
    auto archives = manager->GetArchives();
    std::vector<std::shared_ptr<Ship::Archive>> order;
    std::vector<size_t> slots;
    std::vector<std::shared_ptr<Ship::Archive>> mods;

    sEntries.clear();
    for (const auto& archive : *archives) {
        const std::string& name = archive->GetManifest().Name;
        if (name.empty()) {
            order.push_back(archive);
            continue;
        }
        sEntries.push_back({ archive, IsEnabled(name), Priority(name) });
        if (sEntries.back().enabled) {
            slots.push_back(order.size());
            order.push_back(archive);
            mods.push_back(archive);
        } else {
            sEntries.back().status = "Disabled";
        }
    }
    std::stable_sort(mods.begin(), mods.end(), [](const auto& a, const auto& b) {
        return Priority(a->GetManifest().Name) < Priority(b->GetManifest().Name);
    });
    for (size_t i = 0; i < slots.size(); i++) {
        order[slots[i]] = mods[i];
    }
    std::stable_sort(sEntries.begin(), sEntries.end(),
                     [](const Entry& a, const Entry& b) { return a.priority < b.priority; });

    if (order != *archives) {
        manager->SetArchives(std::make_shared<std::vector<std::shared_ptr<Ship::Archive>>>(order));
    }
}

std::vector<ModList::Entry>& ModList::Entries() {
    return sEntries;
}

bool ModList::IsEnabled(const std::string& name) {
    return CVarGetInteger(Key("Disabled", name).c_str(), 0) == 0;
}

void ModList::SetEnabled(const std::string& name, bool enabled) {
    CVarSetInteger(Key("Disabled", name).c_str(), enabled ? 0 : 1);
    CVarSave();
}

int ModList::Priority(const std::string& name) {
    return CVarGetInteger(Key("Priority", name).c_str(), 0);
}

void ModList::SetPriority(const std::string& name, int priority) {
    CVarSetInteger(Key("Priority", name).c_str(), priority);
    CVarSave();
}

void ModList::ClearStatus() {
    for (Entry& entry : sEntries) {
        if (entry.enabled) {
            entry.status.clear();
            entry.failed = false;
        }
    }
}

void ModList::SetStatus(const std::string& name, const std::string& status, bool failed) {
    for (Entry& entry : sEntries) {
        if (entry.archive->GetManifest().Name == name) {
            entry.status = status;
            entry.failed = failed;
        }
    }
}
