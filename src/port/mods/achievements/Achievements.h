#pragma once

#include "types.h"

struct Achievement {
    std::string id;
    std::string name;
    const char* icon;
    std::string description;
    std::vector<std::string> dependencies;
    int32_t maxProgress = 0;
};

extern std::vector<Achievement> gAchievementList;