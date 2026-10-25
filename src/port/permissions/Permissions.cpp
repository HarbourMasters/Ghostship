#ifdef ENABLE_SCRIPTING
#include "port/permissions/Permissions.h"
#include "port/ui/GhostshipGui.hpp"

#include <ship/Context.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace {

std::string FilePath() {
    return Ship::Context::GetPathRelativeToAppDirectory("permissions.json");
}

std::mutex sStoreMtx;
nlohmann::json sStore = nlohmann::json::object();
bool sLoaded = false;

void Load() {
    if (sLoaded) {
        return;
    }
    sLoaded = true;
    std::ifstream in(FilePath());
    if (!in) {
        return;
    }
    sStore = nlohmann::json::parse(in, nullptr, false);
    if (!sStore.is_object()) {
        SPDLOG_WARN("Permissions: {} is not valid, every permission will be asked again", FilePath());
        sStore = nlohmann::json::object();
    }
}

Permissions::State ReadState(const std::string& key) {
    std::lock_guard<std::mutex> lock(sStoreMtx);
    Load();
    const auto it = sStore.find(key);
    if (it == sStore.end() || !it->is_number_integer()) {
        return Permissions::State::Pending;
    }
    const int value = it->get<int>();
    return value == 1   ? Permissions::State::Allowed
           : value == 2 ? Permissions::State::Denied
                        : Permissions::State::Pending;
}

void WriteState(const std::string& key, Permissions::State state) {
    std::lock_guard<std::mutex> lock(sStoreMtx);
    Load();
    sStore[key] = state == Permissions::State::Allowed ? 1 : 2;
    std::ofstream out(FilePath(), std::ios::trunc);
    if (!out) {
        SPDLOG_ERROR("Permissions: cannot write {}", FilePath());
        return;
    }
    out << sStore.dump(2);
}

std::mutex sPromptMtx;
std::unordered_map<std::string, Permissions::PromptDef> sPrompts;

} // namespace

namespace Permissions {

void Register(const std::string& key, PromptDef prompt) {
    std::lock_guard<std::mutex> lock(sPromptMtx);
    sPrompts[key] = std::move(prompt);
}

State Get(const std::string& key) {
    return ReadState(key);
}

void Request(const std::string& key, std::function<void()> onAllow, std::function<void()> onDeny) {
    if (ReadState(key) != State::Pending) {
        return;
    }

    PromptDef prompt;
    {
        std::lock_guard<std::mutex> lock(sPromptMtx);
        const auto it = sPrompts.find(key);
        if (it == sPrompts.end()) {
            SPDLOG_WARN("Permissions: no prompt registered for '{}'", key);
            return;
        }
        prompt = it->second;
    }

    GhostshipGui::RegisterPopup(
        prompt.title, prompt.message, "Allow", "Deny",
        [key, onAllow]() {
            WriteState(key, State::Allowed);
            SPDLOG_INFO("Permissions: '{}' allowed", key);
            if (onAllow) {
                onAllow();
            }
        },
        [key, onDeny]() {
            WriteState(key, State::Denied);
            SPDLOG_INFO("Permissions: '{}' denied", key);
            if (onDeny) {
                onDeny();
            }
        });
}

} // namespace Permissions
#endif
