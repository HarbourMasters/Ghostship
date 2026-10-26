#include <libultraship.h>
#include <spdlog/spdlog.h>

#include <string>
#include <unordered_map>

#include "port/ShipCompat.h"
#include "port/ShipInit.hpp"
#include "port/events/Events.h"

#include "macros.h"
#include "types.h"

#define STUB_LEVEL(...)
#define DEFINE_LEVEL(_0, _1, _2, folder, ...) extern_s const LevelScript level_##folder##_entry[];
#include "levels/level_defines.h"
#undef DEFINE_LEVEL

namespace {

struct LevelEntry {
    const void* script;
    const char* folder;
};

#define DEFINE_LEVEL(_0, _1, _2, folder, ...) { level_##folder##_entry, #folder },
const LevelEntry kLevels[] = {
#include "levels/level_defines.h"
};
#undef DEFINE_LEVEL
#undef STUB_LEVEL

void OnLevelScriptOverride(IEvent* event) {
    auto* ev = reinterpret_cast<LevelScriptOverride*>(event);
    for (const LevelEntry& level : kLevels) {
        if (*ev->newScript != level.script) {
            continue;
        }
        const std::string path = std::string("scripts/levels/") + level.folder;
        if (!ShipCompat::GetResourceManager()->GetArchiveManager()->HasFile(path)) {
            return;
        }
        if (const void* script = ResourceGetDataByName(path.c_str())) {
            SPDLOG_INFO("Level {}: running {} from the archives", level.folder, path);
            *ev->newScript = script;
        } else {
            SPDLOG_ERROR("Level {}: {} didn't load, running the original level", level.folder, path);
        }
        return;
    }
}

} // namespace

extern "C" s16 gCurrLevelNum;

#define DEFINE_LEVEL(_0, levelenum, _2, folder, ...) { levelenum, #folder },
#define STUB_LEVEL(...)
static const struct {
    s16 num;
    const char* folder;
} kLevelFolders[] = {
#include "levels/level_defines.h"
};
#undef DEFINE_LEVEL
#undef STUB_LEVEL

// textures/skyboxes/levels/<folder>/ptrlist for the current level, or null.
extern "C" const char* Port_LevelSkybox(void) {
    static s16 sLevel = -1;
    static std::string sPath;
    static bool sFound = false;
    if (gCurrLevelNum != sLevel) {
        sLevel = gCurrLevelNum;
        sFound = false;
        for (const auto& level : kLevelFolders) {
            if (level.num == gCurrLevelNum) {
                const std::string path = std::string("textures/skyboxes/levels/") + level.folder + "/ptrlist";
                sFound = ShipCompat::GetResourceManager()->GetArchiveManager()->HasFile(path);
                sPath = "__OTR__" + path;
                break;
            }
        }
    }
    return sFound ? sPath.c_str() : nullptr;
}

// levels/<folder>/<folder>_water_<id> for the current level, or null.
extern "C" const char* Port_LevelWaterBoxes(u32 id) {
    static s16 sLevel = -1;
    static std::unordered_map<u32, std::string> sPaths;
    if (gCurrLevelNum != sLevel) {
        sLevel = gCurrLevelNum;
        sPaths.clear();
    }
    auto it = sPaths.find(id);
    if (it == sPaths.end()) {
        std::string found;
        for (const auto& level : kLevelFolders) {
            if (level.num == gCurrLevelNum) {
                char name[16];
                snprintf(name, sizeof(name), "_water_%04X", id);
                const std::string path = std::string("levels/") + level.folder + "/" + level.folder + name;
                if (ShipCompat::GetResourceManager()->GetArchiveManager()->HasFile(path)) {
                    found = "__OTR__" + path;
                }
                break;
            }
        }
        it = sPaths.emplace(id, found).first;
    }
    return it->second.empty() ? nullptr : it->second.c_str();
}

static RegisterShipInitFunc sLevelOverridesInit([] {
    REGISTER_LISTENER(LevelScriptOverride, EVENT_PRIORITY_NORMAL, OnLevelScriptOverride);
});
