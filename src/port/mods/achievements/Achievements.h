#pragma once

#include "types.h"

typedef struct AchievementSaveEntry {
    const char* id;
    int32_t progress;
} AchievementSaveEntry;

struct AchievementSaveData {
    bool cheated;
    AchievementSaveEntry entries[100];
};

typedef struct AchievementProgress {
    int32_t progress;
    bool achieved;
} AchievementProgress;

#define HAS_ACHIEVEMENTS(fileNum) (gSaveBuffer.files[fileNum]->shipSaveData.features.achievements && gCurrDemoInput == NULL)

#ifdef __cplusplus
#include <map>

enum class AchievementCategory {
    None      = 0,
    Stars   = 1 << 0,
    Caps    = 1 << 1,
    Levels  = 1 << 2,
    Bosses  = 1 << 3,
    Deaths  = 1 << 4,
    Extras  = 1 << 5,
};

struct Achievement {
    AchievementCategory category;
    std::string name;
    const char* icon;
    std::string description;
    size_t order;
    std::vector<std::string> dependencies;
    int32_t maxProgress = 1;
};

extern std::unordered_map<std::string, Achievement> gAchievementList;

extern AchievementProgress* Achievement_GetProgress(const std::string& id);
#endif