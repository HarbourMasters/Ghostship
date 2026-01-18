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

DEFINE_EVENT(SpawnObject,
    int16_t model;
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
    int16_t model;
);

DEFINE_EVENT(OnGameFileLoad,
    s32 fileNum;
);

DEFINE_EVENT(OnGameFileSave,
    s32 fileNum;
);
