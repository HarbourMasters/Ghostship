#pragma once

#include "types.h"
#include "level_table.h"

struct BetterLevelSelect {
    int topDisplayedLevel = 0;
    int currentLevelIndex = 0;
    int currentActIndex = 0;
    int ttcSpeedIndex = 0;
    int timerUp = 0;
    int timerDown = 0;
    bool lockUp = false;
    bool lockDown = false;
    int verticalInput = 0;
    int verticalInputAccumulator = 0;
    bool forceReload = false;
    bool loaded = false;
    int update_rate = 3;
    Gfx pool[0x1024] = {0};
};

struct LevelSelectEntry {
    const char* englishName;
    const char* japaneseName;
    const char* actsEn[6];
    const char* actsJp[6];
    LevelNum areaId;
};

void BetterLevelSelect_HandleReload();