#ifdef ENABLE_SCRIPTING

#include "port/api/ModOwner.hpp"

#include <libultraship/bridge/eventsbridge.h>
#include <ship/events/Events.h>

#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

struct ScopeEntry {
    const std::string* mod;
    const char* listener;
};

std::vector<ScopeEntry>& Scopes() {
    static std::vector<ScopeEntry> scopes;
    return scopes;
}

const std::string* Intern(const std::string& mod) {
    static std::unordered_set<std::string> names;
    return &*names.insert(mod).first;
}

std::unordered_map<std::string, std::shared_ptr<bool>>& AliveFlags() {
    static std::unordered_map<std::string, std::shared_ptr<bool>> flags;
    return flags;
}

std::vector<std::function<void(const std::string&)>>& Releasers() {
    static std::vector<std::function<void(const std::string&)>> releasers;
    return releasers;
}

const char* ActiveListener() {
    const auto events = EventSystemGetEvents();
    return events != nullptr ? events->GetActiveListenerPath() : nullptr;
}

std::string ModFromPath(const char* path) {
    if (path == nullptr || path[0] != '[') {
        return {};
    }
    const char* end = strstr(path, "]:");
    return end != nullptr ? std::string(path + 1, end) : std::string();
}

} // namespace

std::string ModOwner::Current() {
    const char* listener = ActiveListener();
    if (!Scopes().empty() && Scopes().back().listener == listener) {
        return *Scopes().back().mod;
    }
    return ModFromPath(listener);
}

ModOwner::Scope::Scope(const std::string& mod) {
    Scopes().push_back({ Intern(mod), ActiveListener() });
}

ModOwner::Scope::~Scope() {
    Scopes().pop_back();
}

void ModOwner::AddReleaser(std::function<void(const std::string& mod)> releaser) {
    Releasers().push_back(std::move(releaser));
}

void ModOwner::Release(const std::string& mod) {
    if (mod.empty()) {
        return;
    }
    Scope scope(mod);
    for (const auto& releaser : Releasers()) {
        releaser(mod);
    }
    if (auto it = AliveFlags().find(mod); it != AliveFlags().end()) {
        *it->second = false;
        AliveFlags().erase(it);
    }
}

std::shared_ptr<const bool> ModOwner::Alive(const std::string& mod) {
    static const auto game = std::make_shared<const bool>(true);
    if (mod.empty()) {
        return game;
    }
    auto& flag = AliveFlags()[mod];
    if (flag == nullptr) {
        flag = std::make_shared<bool>(true);
    }
    return flag;
}

#endif
