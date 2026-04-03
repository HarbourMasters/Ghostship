#pragma once

#include "include/types.h"
#include "game/area.h"

typedef enum CapSwitchType {
    CAP_SWITCH_WING,
    CAP_SWITCH_METAL,
    CAP_SWITCH_VANISH,
} CapSwitchType;

typedef enum BossType {
    BOSS_TYPE_KING_BOBOMB,
    BOSS_TYPE_KING_WHOMP,
    BOSS_TYPE_BIG_BOO_HUNT,
    BOSS_TYPE_BIG_BOO_MERRY_GO_ROUND,
    BOSS_TYPE_BIG_BOO_BALCONY,
    BOSS_TYPE_BIG_BULLY,
    BOSS_TYPE_EYEROK,
    BOSS_TYPE_WIGGLER,
    BOSS_TYPE_CHILL_BULLY,
    BOSS_TYPE_MR_I,
    BOSS_TYPE_BOWSER_BITDW,
    BOSS_TYPE_BOWSER_BITFS,
    BOSS_TYPE_BOWSER_BITS,
} BossType;

typedef enum BossBattleType {
    BOSS_BATTLE_NONE,
    BOSS_BATTLE_KOOPA,
    BOSS_BATTLE_KOOPA_FINAL,
    BOSS_BATTLE_GENERIC,
} BossBattleType;

typedef enum CollectibleType {
    COLLECTIBLE_TYPE_GRAND_STAR,
    COLLECTIBLE_TYPE_KEY,
} CollectibleType;

DEFINE_EVENT(CapSwitchActivated,
    CapSwitchType type;
);

DEFINE_EVENT(ChainChompRelease,
    struct Object* entity;
);

DEFINE_EVENT(BossDefeated,
    struct Object* entity;
    BossType type;
);

DEFINE_EVENT(SpawnCollectible,
    struct Object* entity;
    CollectibleType itemType;
);

DEFINE_EVENT(BossBattleStarted, 
    BossBattleType type;
);
DEFINE_EVENT(BossBattleEnded);

DEFINE_EVENT(MusicChanged,
    s16 seqId;
);

DEFINE_EVENT(GameEnded);