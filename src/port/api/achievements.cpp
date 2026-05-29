#include "achievements.h"
#include "port/mods/achievements/Achievements.h"

extern "C" void C_RegisterAchievement(const char* id, const C_AchievementDef* def) {
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
    if (!id) {
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
