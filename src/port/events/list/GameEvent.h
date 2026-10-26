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

typedef enum GameValueId {
    GAME_VALUE_TOAD_STAR_1,
    GAME_VALUE_TOAD_STAR_2,
    GAME_VALUE_TOAD_STAR_3,
    GAME_VALUE_MIPS_STAR_1,
    GAME_VALUE_MIPS_STAR_2,
    GAME_VALUE_CANNON_GRATE_STARS,
    GAME_VALUE_CASTLE_BOO_STARS,
    GAME_VALUE_CASTLE_BOO_CAGE,
    GAME_VALUE_KING_BOBOMB_HEALTH,
    GAME_VALUE_STARTING_LIVES,
    GAME_VALUE_PSS_SLIDE_FRAMES,
    GAME_VALUE_VANISH_CAP_MUSIC,
    GAME_VALUE_SKIP_INTRO,
    GAME_VALUE_COURSE_ENTRY_DIALOGS,
    GAME_VALUE_EXIT_COURSE_LEVEL,
    GAME_VALUE_EXIT_COURSE_AREA,
    GAME_VALUE_EXIT_COURSE_NODE,
    GAME_VALUE_KOOPA_SHELL_FLAMES,
    GAME_VALUE_CCM_TREES_SNOW,
    GAME_VALUE_WING_CAP_LIGHT_STARS,
    GAME_VALUE_SUN_WARP_STARS,
    GAME_VALUE_BREAKABLE_BOX_SETS_MODEL,
    GAME_VALUE_BREAKABLE_BOX_MARIO_ACTION, // 0: none
    GAME_VALUE_KING_WHOMP_POUND_FRAMES,
    GAME_VALUE_SURFACE_OBJECTS_ALWAYS_DRAWN,
    GAME_VALUE_CAM_MARIO_SELECTED_FLAGS,
    GAME_VALUE_CAM_SELECTION_KEEP_MASK,
    GAME_VALUE_CAM_MARIO_ACTIVE_ZOOM,
    GAME_VALUE_CAM_INITIAL_MOVEMENT,
    GAME_VALUE_CAM_C_SIDE_YAW,
    GAME_VALUE_CAM_C_SIDE_HELD,
    GAME_VALUE_CAM_C_SIDE_SOUND,
    GAME_VALUE_CAM_AUTO_YAW,
    GAME_VALUE_CAM_ZOOMED_OUT_ALWAYS_HALVED,
    GAME_VALUE_CAM_STEADY_FLOOR_ACTION_1, // 0: none
    GAME_VALUE_CAM_STEADY_FLOOR_ACTION_2,
    GAME_VALUE_FILE_START_LEVEL,
} GameValueId;

typedef enum GameFloatId {
    GAME_FLOAT_HANG_MAX_SPEED,
    GAME_FLOAT_KING_WHOMP_POUND_DIST,
    GAME_FLOAT_KING_WHOMP_JUMP_VEL,
    GAME_FLOAT_CULLING_FOV_SCALE,
    GAME_FLOAT_CAM_ZOOMED_OUT_DIVISOR,
    GAME_FLOAT_BULLY_LAVA_HEIGHT, // 0: none
} GameFloatId;

typedef enum FixedStarId {
    FIXED_STAR_KING_BOBOMB,
    FIXED_STAR_AT_Y_OFFSET, // cur_obj_spawn_star_at_y_offset
    FIXED_STAR_MR_I,
    FIXED_STAR_BIG_BOO_GHOST_HUNT,
    FIXED_STAR_BIG_BOO_BALCONY,
    FIXED_STAR_BIG_BOO_MERRY_GO_ROUND,
    FIXED_STAR_CHILL_BULLY,
    FIXED_STAR_BIG_BULLY,
    FIXED_STAR_FIRE_PIRANHA,
    FIXED_STAR_KING_WHOMP,
    FIXED_STAR_WIGGLER,
    FIXED_STAR_EYEROK,
    FIXED_STAR_PSS_SLIDE,
    FIXED_STAR_KOOPA_RACE_BOB,
    FIXED_STAR_KOOPA_RACE_THI,
    FIXED_STAR_BIG_BULLY_WITH_MINIONS,
} FixedStarId;

typedef enum TrajectoryId {
    TRAJECTORY_KOOPA_RACE_BOB,
    TRAJECTORY_KOOPA_RACE_THI,
    TRAJECTORY_BOWLING_BALL_BOB_UPPER,
    TRAJECTORY_BOWLING_BALL_THI_LARGE,
    TRAJECTORY_RACING_PENGUIN,
    TRAJECTORY_PLATFORM_ON_TRACK, // + path index
} TrajectoryId;

// Fires in game_value. Write *value to change it.
DEFINE_EVENT(GameValue,
    s32  id; // GameValueId
    s32* value;
);

// Fires in game_float. Write *value to change it.
DEFINE_EVENT(GameFloat,
    s32  id; // GameFloatId
    f32* value;
);

// Fires in spawn_fixed_star. Cancellable.
// Write pos to move the star. Cancel — no star is spawned.
DEFINE_EVENT(SpawnFixedStar,
    s32  id; // FixedStarId
    f32* pos; // x, y, z
);

// Fires in game_trajectory. Write *path to replace the trajectory.
DEFINE_EVENT(ModifyTrajectory,
    s32               id; // TrajectoryId
    const Trajectory** path;
);

// Fires in update_camera before camera_course_processing. Cancellable.
// Cancel — skips camera_course_processing this frame.
DEFINE_EVENT(CameraCourseProcessing,
    struct Camera* camera;
);

// Fires in geo_draw_mario_head_goddard. Cancellable.
// Cancel — the head isn't drawn this frame.
DEFINE_EVENT(TitleScreenFrame);

// Fires in print_displaying_credits_entry. Write *lines to replace the entry's text.
DEFINE_EVENT(CreditsEntryLines,
    s32           entry;
    const char*** lines;
);
