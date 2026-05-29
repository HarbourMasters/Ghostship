#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

// Achievement categories — mirrors AchievementCategory in Achievements.h.
typedef enum {
    C_ACH_CATEGORY_NONE   = 0,
    C_ACH_CATEGORY_STARS  = 1 << 0,
    C_ACH_CATEGORY_CAPS   = 1 << 1,
    C_ACH_CATEGORY_LEVELS = 1 << 2,
    C_ACH_CATEGORY_BOSSES = 1 << 3,
    C_ACH_CATEGORY_DEATHS = 1 << 4,
    C_ACH_CATEGORY_EXTRAS = 1 << 5,
} C_AchievementCategory;

// Definition passed to C_RegisterAchievement.
// dependencies: null-terminated array of achievement IDs this one depends on,
//               or NULL if there are none.
// maxProgress:  goal value — use 1 for a binary (done/not-done) achievement.
typedef struct {
    C_AchievementCategory  category;
    const char*            name;
    const char*            icon;        // texture path, e.g. "extras.my-ach"
    const char*            description;
    int32_t                maxProgress;
    const char**           dependencies;
} C_AchievementDef;

// Register a new achievement. Safe to call at any point after engine init
// (e.g. inside an INIT_FUNC block). Registering the same id twice is a no-op.
extern_s void    C_RegisterAchievement(const char* id, const C_AchievementDef* def);

// Increment progress on an achievement by `amount`.
// Use amount=1 for binary achievements.
extern_s void    C_AchievementProgress(const char* id, int32_t amount);

// Returns current progress value (0 if the id is unknown).
extern_s int32_t C_AchievementGetProgress(const char* id);

// Returns 1 if the achievement is unlocked, 0 otherwise.
extern_s int     C_AchievementIsUnlocked(const char* id);
