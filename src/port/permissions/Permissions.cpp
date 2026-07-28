#ifndef __SWITCH__
#include "Permissions.h"

#include "port/ui/GhostshipGui.hpp"
#include "ship/core/Context.h"
#include "spdlog/spdlog.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace {

std::string PermissionsFilePath() {
    return Ship::Context::GetPathRelativeToAppDirectory("permissions.json");
}

struct FileStore {
    std::mutex mtx;
    nlohmann::json data;
    bool loaded = false;

    void Load() {
        if (loaded) {
            return;
        }

        loaded = true;

        const std::string path = PermissionsFilePath();
        if (!std::filesystem::exists(path)) {
            return;
        }

        std::ifstream f(path);
        if (!f.is_open()) {
            SPDLOG_WARN("Permissions: cannot open '{}' for reading", path);
            return;
        }

        try {
            data = nlohmann::json::parse(f);
        } catch (const std::exception& e) {
            SPDLOG_WARN("Permissions: failed to parse file: {}", e.what());
            data = nlohmann::json::object();
        }
    }

    void Flush() {
        const std::string path = PermissionsFilePath();
        std::ofstream f(path);
        if (!f.is_open()) {
            SPDLOG_ERROR("Permissions: cannot open '{}' for writing", path);
            return;
        }
        f << data.dump(2);
    }
} gStore;

Permissions::State ReadState(const std::string& key) {
    std::lock_guard<std::mutex> lock(gStore.mtx);
    gStore.Load();

    if (!gStore.data.contains(key)) {
        return Permissions::State::Pending;
    }

    int v = gStore.data[key].get<int>();

    if (v == 1) {
        return Permissions::State::Allowed;
    }

    if (v == 2) {
        return Permissions::State::Denied;
    }

    return Permissions::State::Pending;
}

void WriteState(const std::string& key, Permissions::State state) {
    std::lock_guard<std::mutex> lock(gStore.mtx);
    gStore.Load();
    gStore.data[key] = (state == Permissions::State::Allowed) ? 1 : 2;
    gStore.Flush();
}

std::mutex gPromptMtx;
std::unordered_map<std::string, Permissions::PromptDef> gPrompts;

} // anonymous namespace

namespace Permissions {

void Register(const std::string& key, PromptDef prompt) {
    std::lock_guard<std::mutex> lock(gPromptMtx);
    gPrompts[key] = std::move(prompt);
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
        std::lock_guard<std::mutex> lock(gPromptMtx);
        auto it = gPrompts.find(key);
        if (it == gPrompts.end()) {
            SPDLOG_WARN("Permissions: no prompt registered for key '{}'", key);
            return;
        }
        prompt = it->second;
    }

    GhostshipGui::RegisterPopup(
        prompt.title, prompt.message, "Allow", "Deny",
        [key, onAllow]() {
            WriteState(key, State::Allowed);
            SPDLOG_INFO("Permissions: '{}' allowed by user", key);
            if (onAllow) {
                onAllow();
            }
        },
        [key, onDeny]() {
            WriteState(key, State::Denied);
            SPDLOG_INFO("Permissions: '{}' denied by user", key);
            if (onDeny) {
                onDeny();
            }
        });
}

} // namespace Permissions
#endif
