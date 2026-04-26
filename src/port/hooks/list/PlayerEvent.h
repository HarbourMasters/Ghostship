#pragma once

#include "port/hooks/impl/EventSystem.h"
#include "include/types.h"
#include "game/area.h"

typedef enum PlayerDeathType {
    DEATH_TYPE_DEFAULT,
    DEATH_TYPE_FALL,
    DEATH_TYPE_LAVA,
    DEATH_TYPE_FIRE,
    DEATH_TYPE_QUICKSAND,
    DEATH_TYPE_EATEN,
    DEATH_TYPE_SQUISHED,
    DEATH_TYPE_DROWNING,
    DEATH_TYPE_WHIRLPOOL,
    DEATH_TYPE_OUT_OF_BOUNDS,
} PlayerDeathType;

DEFINE_EVENT(PlayerHealthChange,
    struct MarioState* m;
    s32 health;
);

DEFINE_EVENT(PlayerLivesChange,
    struct MarioState* m;
    s32 lives;
);

DEFINE_EVENT(PlayerStartedDialog,
    struct MarioState* m;
    int32_t dialogId;
);

DEFINE_EVENT(PlayerDeath,
    struct MarioState* m;
    PlayerDeathType type;
);

DEFINE_EVENT(PlayerExecuteAction,
    struct MarioState* m;
    u32 action;
    u32 arg;
);

DEFINE_EVENT(SetTripleJumpAction,
    struct MarioState* m;
    bool* useFlyingVariant;
);

DEFINE_EVENT(FlyingActionUpdate,
    struct MarioState* m;
    bool* canFly;
);

DEFINE_EVENT(FlyingTripleJumpLaunch,
    struct MarioState* m;
    f32* launchVelocity;
);

DEFINE_EVENT(ItemCollected,
    int16_t type;
    struct MarioState* marioState;
    struct Object* object;
);

DEFINE_EVENT(MacroObjectOverride,
    int16_t model;
    int16_t posX;
    int16_t posY;
    int16_t posZ;
);

DEFINE_EVENT(SpawnCoinStar,
    int16_t posX;
    int16_t posY;
    int16_t posZ;
);

DEFINE_EVENT(SpawnStar,
    int16_t* model;
    f32 posX;
    f32 posY;
    f32 posZ;
);

DEFINE_EVENT(ModifyDefaultStar,
    f32 posX;
    f32 posY;
    f32 posZ;
    int32_t param;
);

DEFINE_EVENT(ModifyObjectBehavior,
    struct Object* object;
    int16_t model;
);

DEFINE_EVENT(ModifyRedCoinCount,
    int8_t* redCoinsCollected;
);

DEFINE_EVENT(ModifyObjectVisibility,
    struct Object* object;
);

DEFINE_EVENT(ChangeLevel,
    int16_t sourceWarpNode;
    struct WarpNode* warpNode;
    int32_t* delayedWarpArg;
);

DEFINE_EVENT(ExitLevel,
    int16_t menuOption;
);