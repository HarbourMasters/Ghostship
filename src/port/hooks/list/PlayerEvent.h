#pragma once

#include "port/hooks/impl/EventSystem.h"
#include "include/types.h"

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
);