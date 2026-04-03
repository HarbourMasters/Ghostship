#pragma once

#ifdef __cplusplus
#include <string>
#include <unordered_map>

#include "loader.h"
#include "port/importer/types/Text.h"

class ScriptingLayer {
  public:
    static ScriptingLayer* Instance;

    void Load(std::string file);
    void Load(const std::string& path, const std::shared_ptr<Ship::Archive>& archive);
    void Clean();
    void Reload();
private:
    std::unordered_map<std::string, ModInstance> instances;
};
extern "C" {
#endif

void BindEvent(const char* name, int32_t id);

#ifdef __cplusplus
}
#endif