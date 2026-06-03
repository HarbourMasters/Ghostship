#pragma once

#include "include/types.h"
#include "game/area.h"
#include "game/paintings.h"

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

// ────────────────────────────────────────────────────────────────────────────
// Game lifecycle
// ────────────────────────────────────────────────────────────────────────────

// Fires when the ending credits complete and the game is considered finished.
DEFINE_EVENT(GameEnded);

// ────────────────────────────────────────────────────────────────────────────
// Objects
// ────────────────────────────────────────────────────────────────────────────

// Fires at the end of create_object() for every newly allocated game object,
// before any behavior script runs.
DEFINE_EVENT(ObjectSpawned,
    struct Object* object;
);

// Fires inside mark_obj_for_deletion() before activeFlags is cleared.
// The object is still fully valid at this point.
DEFINE_EVENT(ObjectDestroyed,
    struct Object* object;
);

// Fires at the start of cur_obj_update() for every active object, every frame,
// before the behavior script executes. High-frequency — keep listeners fast.
DEFINE_EVENT(BehaviorTick,
    struct Object* object;
);

// ────────────────────────────────────────────────────────────────────────────
// Progress
// ────────────────────────────────────────────────────────────────────────────

// Fires only when a genuinely new star is saved (already-owned stars do not fire this).
// courseNum is 0-based (COURSE_NUM_TO_INDEX result). starIndex is the slot (0-based).
DEFINE_EVENT(StarCollected,
    s32 courseNum;
    s32 starIndex;
);

// ────────────────────────────────────────────────────────────────────────────
// Boss encounters
// ────────────────────────────────────────────────────────────────────────────

// Fires when a boss battle begins.
DEFINE_EVENT(BossBattleStarted,
    BossBattleType type;
);

// Fires when any boss battle ends (after defeat sequence or abort).
DEFINE_EVENT(BossBattleEnded);

// Fires when a boss's defeat condition is met. Cancellable.
// Cancel — prevents the defeat sequence; boss stays alive.
DEFINE_EVENT(BossDefeated,
    struct Object* entity;
    BossType       type;
);

// Fires when a grand star or castle key is about to spawn after a Bowser fight. Cancellable.
// Cancel — suppresses spawning.
DEFINE_EVENT(SpawnCollectible,
    struct Object*  entity;
    CollectibleType itemType; // COLLECTIBLE_TYPE_GRAND_STAR or COLLECTIBLE_TYPE_KEY
);

// ────────────────────────────────────────────────────────────────────────────
// World objects
// ────────────────────────────────────────────────────────────────────────────

// Fires when Mario presses a cap switch. Cancellable.
// Cancel — suppresses activating the switch.
DEFINE_EVENT(CapSwitchActivated,
    CapSwitchType type; // CAP_SWITCH_WING / CAP_SWITCH_METAL / CAP_SWITCH_VANISH
);

// Fires when the chain chomp breaks its post and is freed. Cancellable.
// Cancel — keeps the chomp chained.
DEFINE_EVENT(ChainChompRelease,
    struct Object* entity;
);

// ────────────────────────────────────────────────────────────────────────────
// Music
// ────────────────────────────────────────────────────────────────────────────

// Fires when the background music sequence changes.
DEFINE_EVENT(MusicChanged,
    s16 seqId; // new sequence ID (SEQ_*)
);

// ────────────────────────────────────────────────────────────────────────────
// Camera / cutscene
// ────────────────────────────────────────────────────────────────────────────

// Fires once when c->cutscene transitions from 0 to a non-zero ID.
DEFINE_EVENT(CutsceneStart,
    struct Camera* c;
    s16 cutsceneId; // new cutscene constant (e.g. CUTSCENE_STAR_SPAWN)
);

// Fires once when c->cutscene returns to 0.
DEFINE_EVENT(CutsceneEnd,
    s16 cutsceneId; // ID of the cutscene that just ended
);

// ────────────────────────────────────────────────────────────────────────────
// Warp / navigation
// ────────────────────────────────────────────────────────────────────────────

// Fires at the end of initiate_warp() after sWarpDest is fully populated.
// The warp is queued but not yet executed.
DEFINE_EVENT(WarpStart,
    s16 destLevel;
    s16 destArea;
    s16 warpNode;
);

// Fires inside init_mario_after_warp() after the destination area resets,
// just before sWarpDest is cleared.
DEFINE_EVENT(WarpEnd,
    s16 level; // gCurrLevelNum at arrival
    s16 area;  // gCurrAreaIndex at arrival
);

// Fires per controller per frame when one or more new buttons are pressed
// (rising edge only — held buttons do not repeat).
DEFINE_EVENT(ButtonPressed,
    struct Controller* controller;
    u16 button; // bitmask of newly pressed buttons (A_BUTTON | B_BUTTON etc.)
);

// ────────────────────────────────────────────────────────────────────────────
// Paintings
// ────────────────────────────────────────────────────────────────────────────

// Fires when painting_state(PAINTING_ENTERED, ...) is called (painting warp state).
DEFINE_EVENT(PaintingEntered,
    struct Painting* painting;
);

// Fires when painting_state(PAINTING_RIPPLE, ...) is called (Mario is nearby).
DEFINE_EVENT(PaintingRipple,
    struct Painting* painting;
);

// ────────────────────────────────────────────────────────────────────────────
// World interactions
// ────────────────────────────────────────────────────────────────────────────

// Fires in interact_water_ring. Cancellable.
// Write *healAmount to change the heal increment. Cancel — suppresses the heal.
DEFINE_EVENT(WaterRingPickup,
    struct MarioState* m;
    struct Object*     ring;
    s32*               healAmount; // mutable heal increment (raw, before application)
);

// Fires in interact_tornado wrapping the full pull-in response. Cancellable.
// Cancel — prevents the tornado from affecting Mario.
DEFINE_EVENT(TornadoInteraction,
    struct MarioState* m;
    struct Object*     tornado;
);

// Fires in interact_warp_door before the door action is set. Cancellable.
// Write *doorAction to redirect (e.g. ACT_PULLING_DOOR / ACT_UNLOCKING_KEY_DOOR).
// Cancel — blocks the door interaction entirely.
DEFINE_EVENT(WarpDoorInteraction,
    struct MarioState* m;
    struct Object*     door;
    u32*               doorAction; // mutable action constant for door entry
);
