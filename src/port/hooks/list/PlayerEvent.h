#pragma once

#include "port/hooks/impl/EventSystem.h"
#include "include/types.h"
#include "game/area.h"

DEFINE_EVENT(PlayerHealthChange,
    struct MarioState* m;
    s32 health;
);

DEFINE_EVENT(PlayerLivesChange,
    struct MarioState* m;
    s32 lives;
);
DEFINE_EVENT(SpawnMacroObject,
    const BehaviorScript** behavior;
    s16* model;
    s16 posX;
    s16 posY;
    s16 posZ;
    s32* params;
);

DEFINE_EVENT(SpawnObject,
    u32* model;
    struct SpawnInfo** spawnInfo;
);

DEFINE_EVENT(SpawnStar,
    f32 posX;
    f32 posY;
    f32 posZ;
);

DEFINE_EVENT(OnGameFileLoad,
    s32 fileNum;
);

DEFINE_EVENT(OnGameFileSave,
    s32 fileNum;
);
