#pragma once

#include "types.h"
#include "level_table.h"

struct BetterLevelSelect {
    int topDisplayedLevel = 0;
    int currentLevelIndex = 0;
    int currentActIndex = 0;
    int currentAreaIndex = 0;
    int ttcSpeedIndex = 0;
    int timerUp = 0;
    int timerDown = 0;
    bool lockUp = false;
    bool lockDown = false;
    int verticalInput = 0;
    int verticalInputAccumulator = 0;
    bool forceReload = false;
    bool areaChanged = false;
    bool loaded = false;
    int update_rate = 3;
    Gfx pool[0x4096] = {0};
};

struct LevelArea {
    const char* name;
    int16_t areaId;
    int16_t warpId;
};

struct LevelSelectEntry {
    LevelNum levelId;
    const char* englishName;
    const char* japaneseName;
    std::vector<const char*> actsEn;
    std::vector<const char*> actsJp;
    std::vector<LevelArea> areas;
};

void BetterLevelSelect_HandleReload();