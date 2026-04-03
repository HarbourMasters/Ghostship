#include "scripting.h"
#include <spdlog/spdlog.h>

#include <fstream>
#include <filesystem>
#include <utility>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <libtcc.h>

#include "loader.h"

namespace fs = std::filesystem;

ScriptingLayer* ScriptingLayer::Instance = new ScriptingLayer();

std::optional<std::vector<uint8_t>> LoadFromO2R(const std::string& path,
                                                const std::shared_ptr<Ship::Archive>& archive = nullptr) {
    const auto file = archive->LoadFile(path);
    if (file == nullptr || !file->IsLoaded) {
        SPDLOG_ERROR("Failed to load script file: {}", path);
        return std::nullopt;
    }

    return std::vector<uint8_t>(file->Buffer->begin(), file->Buffer->end());
}

void ScriptingLayer::Load(const std::string& path, const std::shared_ptr<Ship::Archive>& archive) {
    const auto result = LoadFromO2R(path, archive);

    if (!result.has_value()) {
        throw std::runtime_error("Failed to load script file: " + path);
    }

    const std::vector<uint8_t>& raw = result.value();

    TCCState* s = tcc_new();
    if (!s) {
        throw std::runtime_error("Failed to create tcc state");
    }

    tcc_set_output_type(s, TCC_OUTPUT_DLL);

    if (tcc_compile_string(s, reinterpret_cast<const char*>(raw.data())) == -1) {
        tcc_delete(s);
        throw std::runtime_error("Failed to compile " + path);
    }

    ModInstance instance;
    const std::string temp = instance.GenerateTempFile();

    if (tcc_output_file(s, temp.c_str()) == -1) {
        tcc_delete(s);
        throw std::runtime_error("Failed to output compiled code for " + path);
    }

    instance.Init(temp);
    const ModFunc_t init = instance.GetFunction("ModInit");

    if (init) {
        init();
        instances[path] = std::move(instance);
    } else {
        instance.Unload();
        throw std::runtime_error("Failed to find ModInit function in mod: " + path);
    }
}

void ScriptingLayer::Clean() {
    for (auto& [path, instance] : instances) {
        const ModFunc_t exit = instance.GetFunction("ModExit");
        if (exit) {
            exit();
        }
        instance.Unload();
    }
    instances.clear();
}

void ScriptingLayer::Reload() {
    this->Clean();
}