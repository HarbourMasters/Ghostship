#include "port/ModList.h"
#include "port/ShipCompat.h"
#include "port/romhack/Romhack.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>

namespace {

std::vector<ModList::Entry> sEntries;

bool NonEmpty(const nlohmann::json& json, const char* key) {
    if (!json.contains(key)) {
        return false;
    }
    const auto& value = json[key];
    return value.is_string() ? !value.get<std::string>().empty()
                             : (value.is_object() || value.is_array()) && !value.empty();
}

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
        // The romhack has a manifest too, but it is picked as the base game and stays where it was mounted.
        if (name.empty() || archive == Romhack::ActiveArchive()) {
            order.push_back(archive);
            continue;
        }
        sEntries.push_back({ archive, IsEnabled(name), Priority(name) });
        Entry& entry = sEntries.back();
        if (const auto manifest = ModRules::ReadManifest(*archive)) {
            entry.rules = ModRules::Parse(*manifest);
            entry.code = NonEmpty(*manifest, "main") || NonEmpty(*manifest, "binaries");
            entry.shaders = manifest->contains("shaders") && (*manifest)["shaders"].is_object() &&
                            (NonEmpty((*manifest)["shaders"], "passes") || NonEmpty((*manifest)["shaders"], "materials"));
        }
        if (!entry.enabled) {
            entry.status = "Disabled";
            continue;
        }
        // Stays enabled, so it loads again once its base is the active one.
        if (const auto reason = ModRules::IncompatibleReason(entry.rules)) {
            SPDLOG_WARN("Not loading mod {}: {}", name, *reason);
            entry.status = "Not loaded. " + *reason;
            continue;
        }
        entry.mounted = true;
        slots.push_back(order.size());
        order.push_back(archive);
        mods.push_back(archive);
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

void ModList::SetOrder(const std::vector<std::string>& enabledFirstWins, const std::vector<std::string>& disabled) {
    const int count = static_cast<int>(enabledFirstWins.size());
    for (int i = 0; i < count; i++) {
        CVarSetInteger(Key("Disabled", enabledFirstWins[i]).c_str(), 0);
        CVarSetInteger(Key("Priority", enabledFirstWins[i]).c_str(), count - 1 - i);
    }
    for (const std::string& name : disabled) {
        CVarSetInteger(Key("Disabled", name).c_str(), 1);
    }
    CVarSave();
}

void ModList::ClearStatus() {
    for (Entry& entry : sEntries) {
        if (entry.mounted) {
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
