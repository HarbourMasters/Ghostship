#pragma once

#include "types.h"

enum class AchievementCategory {
    None      = 0,
    Stars   = 1 << 0,
    Caps    = 1 << 1,
    Levels  = 1 << 2,
    Bosses  = 1 << 3,
    Deaths  = 1 << 4,
    Extras  = 1 << 5,
};

struct AchievementProgress {
    int32_t progress;
    bool achieved;
};

struct Achievement {
    std::string id;
    AchievementCategory category;
    std::string name;
    const char* icon;
    std::string description;
    std::vector<std::string> dependencies;
    int32_t maxProgress = 1;
};

struct AchievementSaveData {
    std::unordered_map<std::string, AchievementProgress> progress;
    bool cheated;
};

extern std::vector<Achievement> gAchievementList;

extern AchievementProgress* Achievement_GetProgress(const std::string& id);