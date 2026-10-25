#include "achievements.h"
#include "port/api/SizedDef.hpp"
#include "port/mods/achievements/Achievements.h"

extern "C" void C_RegisterAchievementSized(const char* id, const C_AchievementDef* achievement_def, uint32_t size) {
    const SizedDef<C_AchievementDef> sized(achievement_def, size);
    const C_AchievementDef* def = sized.get();
    if (!id || !def || gAchievementList.contains(id)) {
        return;
    }

    Achievement ach;
    ach.category = static_cast<AchievementCategory>(def->category);
    ach.name = def->name ? def->name : "";
    ach.icon = def->icon ? def->icon : "";
    ach.description = def->description ? def->description : "";
    ach.maxProgress = def->maxProgress > 0 ? def->maxProgress : 1;
    ach.order = gAchievementList.size();

    if (def->dependencies) {
        for (const char** dep = def->dependencies; *dep != nullptr; ++dep) {
            ach.dependencies.push_back(*dep);
        }
    }

    gAchievementList[id] = std::move(ach);
    gAchievementProgress[id] = { 0, false };
}

extern "C" void C_AchievementProgress(const char* id, int32_t amount) {
    if (!id || !Achievement_SystemActive()) {
        return;
    }
    Achievement_Progress(id, amount);
}

extern "C" int32_t C_AchievementGetProgress(const char* id) {
    if (!id) {
        return 0;
    }

    AchievementProgress* p = Achievement_GetProgress(id);
    return p ? p->progress : 0;
}

extern "C" int C_AchievementIsUnlocked(const char* id) {
    if (!id) {
        return 0;
    }

    AchievementProgress* p = Achievement_GetProgress(id);
    return (p && p->achieved) ? 1 : 0;
}
