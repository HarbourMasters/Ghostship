#pragma once

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

// ────────────────────────────────────────────────────────────────────────────
// Core action / state machine
// ────────────────────────────────────────────────────────────────────────────

// Fires every time set_mario_action() is called. Cancellable.
// Cancel — blocks the action change; Mario keeps his current action.
DEFINE_EVENT(PlayerSetAction,
    struct MarioState* m;
    u32 action; // proposed action constant (ACT_*)
    u32 arg;    // secondary argument passed to the action
);

// Fires each frame when executing Mario's current action function. Cancellable.
// Write *result to override the return value. Cancel — skips executing the action.
DEFINE_EVENT(PlayerExecuteAction,
    s32* result; // mutable return value of the action function
);

// Fires inside the airborne common-cancel check (ground pound, ledge grab, etc.). Cancellable.
// Write *result to force a specific cancel result. Cancel — skips standard checks.
DEFINE_EVENT(PlayerCheckCommonAirborneCancels,
    struct MarioState* m;
    s32* result; // mutable cancel result
);

// ────────────────────────────────────────────────────────────────────────────
// Stats
// ────────────────────────────────────────────────────────────────────────────

// Fires before any health delta is applied. Cancellable.
// health is the change amount (negative = damage, positive = heal).
// Cancel — blocks the health change.
DEFINE_EVENT(PlayerHealthChange,
    struct MarioState* m;
    s32 health; // delta amount (not the new absolute value)
);

// Fires before a lives delta is applied. Cancellable.
// lives is the change amount (-1 for death, +1 for 1-Up).
// Cancel — blocks the lives change.
DEFINE_EVENT(PlayerLivesChange,
    struct MarioState* m;
    s32 lives; // delta amount
);

// ────────────────────────────────────────────────────────────────────────────
// Death
// ────────────────────────────────────────────────────────────────────────────

// Fires for every death trigger. Cancellable.
// Cancel — prevents the death sequence from starting.
DEFINE_EVENT(PlayerDeath,
    struct MarioState* m;
    PlayerDeathType type; // cause of death
);

// ────────────────────────────────────────────────────────────────────────────
// Dialogue / interaction
// ────────────────────────────────────────────────────────────────────────────

// Fires when Mario enters a dialog interaction.
DEFINE_EVENT(PlayerStartedDialog,
    struct MarioState* m;
    int32_t dialogId; // index into the dialog table
);

// ────────────────────────────────────────────────────────────────────────────
// Item collection
// ────────────────────────────────────────────────────────────────────────────

// Fires when Mario collects a coin (TYPE_COIN) or star (TYPE_STAR). Cancellable.
// Cancel — suppresses collection (item not counted or removed).
DEFINE_EVENT(ItemCollected,
    int16_t type;                  // TYPE_COIN or TYPE_STAR
    struct MarioState* marioState;
    struct Object* object;
);

// ────────────────────────────────────────────────────────────────────────────
// Cap
// ────────────────────────────────────────────────────────────────────────────

// Fires in interact_cap after cap type is resolved. Cancellable.
// Write *capFlag to substitute a different cap. Write *capTime to change duration.
// Cancel — prevents the cap from being applied.
DEFINE_EVENT(PlayerCapGained,
    struct MarioState* m;
    struct Object* source;
    u32* capFlag; // MARIO_WING_CAP / MARIO_METAL_CAP / MARIO_VANISH_CAP
    u16* capTime; // duration in frames
);

// ────────────────────────────────────────────────────────────────────────────
// Object carry
// ────────────────────────────────────────────────────────────────────────────

// Fires in mario_grab_used_object when Mario picks up a holdable object.
DEFINE_EVENT(PlayerObjectGrabbed,
    struct MarioState* m;
    struct Object* object;
);

// Fires in mario_throw_held_object just before the throw trajectory is applied.
DEFINE_EVENT(PlayerObjectThrown,
    struct MarioState* m;
    struct Object* object;
);

// Fires in mario_drop_held_object just before the object is released.
DEFINE_EVENT(PlayerObjectDropped,
    struct MarioState* m;
    struct Object* object;
);

// ────────────────────────────────────────────────────────────────────────────
// Landing / combat
// ────────────────────────────────────────────────────────────────────────────

// Fires when Mario lands on a surface after being airborne.
// fallHeight is the height fallen in units (same value used for fall-damage checks).
DEFINE_EVENT(PlayerLanded,
    struct MarioState* m;
    f32 fallHeight;
);

// Fires after m->hurtCounter is incremented when Mario takes damage from an object.
// damage is the raw value before the x4 multiplier.
DEFINE_EVENT(PlayerHit,
    struct MarioState* m;
    struct Object* source;
    s32 damage; // raw damage value before the ×4 multiplier
);

// Fires at the end of determine_knockback_action() after direction/terrain are resolved.
// Write *action to substitute a different knockback action. Cancellable.
// Cancel — keeps Mario's current action (no knockback).
DEFINE_EVENT(PlayerKnockback,
    struct MarioState* m;
    u32* action; // mutable knockback action constant (ACT_*)
);

// Fires in interact_bounce_top when Mario attacks an enemy with sufficient force. Cancellable.
// Write *interaction to change the attack type. Cancel — suppresses the bounce and attack.
DEFINE_EVENT(PlayerBounceOnEnemy,
    struct MarioState* m;
    struct Object* enemy;
    u32* interaction; // mutable attack type (INT_HIT_FROM_ABOVE etc.)
);

// ────────────────────────────────────────────────────────────────────────────
// Specific interactions
// ────────────────────────────────────────────────────────────────────────────

// Fires in interact_cannon_base after state is set up but before ACT_IN_CANNON. Cancellable.
// Cancel — keeps Mario's current action; cannon is not entered.
DEFINE_EVENT(CannonEntered,
    struct MarioState* m;
    struct Object* cannon;
);

// Fires in interact_pole when Mario first grabs a pole (not on every riding frame). Cancellable.
// Write *lowSpeed to force slow (ACT_GRAB_POLE_SLOW) or fast (ACT_GRAB_POLE_FAST) grab.
// Cancel — prevents the grab entirely.
DEFINE_EVENT(PoleGrabbed,
    struct MarioState* m;
    struct Object* pole;
    u32* lowSpeed; // non-zero = slow grab; zero = fast grab
);

// Fires in interact_hoot before ACT_RIDING_HOOT is set. Cancellable.
// Cancel — prevents the ride.
DEFINE_EVENT(HootGrabbed,
    struct MarioState* m;
    struct Object* hoot;
);

// Fires in interact_koopa_shell after sounds play but before ACT_RIDING_SHELL_GROUND. Cancellable.
// Cancel — prevents mounting; shell stays on the ground.
DEFINE_EVENT(ShellMounted,
    struct MarioState* m;
    struct Object* shell;
);

// Fires in interact_flame after immunity checks pass. Cancellable.
// Write *burningAction to change the burn animation. Cancel — suppresses the burn entirely.
DEFINE_EVENT(FlameHit,
    struct MarioState* m;
    struct Object* flame;
    u32* burningAction; // ACT_BURNING_JUMP or ACT_BURNING_FALL
);

// Fires in interact_shock wrapping the full shock response. Cancellable.
// Cancel — suppresses all shock damage and action change.
DEFINE_EVENT(ShockHit,
    struct MarioState* m;
    struct Object* shock;
);

// Fires in interact_breakable when Mario attacks with sufficient force. Cancellable.
// Write *interaction to change the attack type. Cancel — prevents the object from breaking.
DEFINE_EVENT(BreakableHit,
    struct MarioState* m;
    struct Object* breakable;
    u32* interaction; // mutable attack type (INT_HIT_FROM_ABOVE / INT_HIT_FROM_BELOW etc.)
);

// ────────────────────────────────────────────────────────────────────────────
// Jump subtypes — fire on the first frame of each airborne action
// ────────────────────────────────────────────────────────────────────────────

// Fires on the first frame of act_triple_jump().
DEFINE_EVENT(PlayerTripleJump,
    struct MarioState* m;
);

// Fires on the first frame of act_wall_kick_air().
DEFINE_EVENT(PlayerWallJump,
    struct MarioState* m;
);

// Fires on the first frame of act_long_jump().
// Write *forwardVel to modify the jump distance.
DEFINE_EVENT(PlayerLongJump,
    struct MarioState* m;
    f32* forwardVel; // mutable forward velocity at jump start
);

// Fires on the first frame of act_backflip().
DEFINE_EVENT(PlayerBackflip,
    struct MarioState* m;
);

// ────────────────────────────────────────────────────────────────────────────
// Ground pound
// ────────────────────────────────────────────────────────────────────────────

// Fires on the first frame of act_ground_pound() when the pound begins.
DEFINE_EVENT(PlayerGroundPoundStart,
    struct MarioState* m;
);

// Fires in act_ground_pound() the moment Mario lands after the pound.
DEFINE_EVENT(PlayerGroundPoundLand,
    struct MarioState* m;
    f32 fallHeight; // peakHeight - landPos
);

// ────────────────────────────────────────────────────────────────────────────
// Water
// ────────────────────────────────────────────────────────────────────────────

// Fires on the first frame of act_water_plunge() (Mario enters water).
DEFINE_EVENT(PlayerWaterEntry,
    struct MarioState* m;
);

// Fires on the first frame of act_swimming_end() (Mario breaks the water surface).
DEFINE_EVENT(PlayerWaterExit,
    struct MarioState* m;
);

// ────────────────────────────────────────────────────────────────────────────
// Physics / terrain
// ────────────────────────────────────────────────────────────────────────────

// Fires in perform_ground_step() when m->floor->type changes from the previous frame.
// Write *newType to substitute a different surface type for physics/sound purposes.
DEFINE_EVENT(PlayerFloorTypeChange,
    struct MarioState* m;
    s16  prevType; // surface type from last frame
    s16* newType;  // mutable new surface type
);

// Fires each frame Mario is sinking in quicksand. Cancellable.
// Write *quicksandDepth to control the sink depth directly.
// Cancel — prevents all depth changes this frame (stops sinking).
DEFINE_EVENT(PlayerQuicksandSink,
    struct MarioState* m;
    f32* quicksandDepth; // mutable current sink depth in units
);

// Fires in mario_update_windy_ground() before wind velocity is applied. Cancellable.
// Write *pushX / *pushZ to change the wind force. Cancel — suppresses the wind push.
DEFINE_EVENT(PlayerWindForce,
    struct MarioState* m;
    f32* pushX; // mutable X velocity contribution from wind
    f32* pushZ; // mutable Z velocity contribution from wind
);

// ────────────────────────────────────────────────────────────────────────────
// Rando / object manipulation hooks
// ────────────────────────────────────────────────────────────────────────────

// Fires during macro object table processing. Cancellable.
// Cancel — skips placing this macro object instance.
DEFINE_EVENT(MacroObjectOverride,
    int16_t model;
    int16_t posX;
    int16_t posY;
    int16_t posZ;
);

// Fires when a coin-star collectible is about to appear. Cancellable.
// Cancel — suppresses spawning.
DEFINE_EVENT(SpawnCoinStar,
    int16_t posX;
    int16_t posY;
    int16_t posZ;
);

// Fires when a star object spawns.
// Write *model to substitute a different model.
DEFINE_EVENT(SpawnStar,
    int16_t* model; // mutable model ID
    f32 posX;
    f32 posY;
    f32 posZ;
);

// Fires for each star placed by the default star-spawn behavior. Cancellable.
// Cancel — suppresses this star placement.
DEFINE_EVENT(ModifyDefaultStar,
    f32     posX;
    f32     posY;
    f32     posZ;
    int32_t param;
);

// Fires when an object's behavior initialises its model. Cancellable.
// Cancel — redirects to custom behavior.
DEFINE_EVENT(ModifyObjectBehavior,
    struct Object* object;
    int16_t model; // model ID being assigned
);

// Fires inside the red-coin star logic before the count is checked. Cancellable.
// Write *redCoinsCollected to override the total.
DEFINE_EVENT(ModifyRedCoinCount,
    int8_t* redCoinsCollected;
);

// Fires when object visibility is being determined.
DEFINE_EVENT(ModifyObjectVisibility,
    struct Object* object;
);

// Fires when a warp triggers a full level change.
// Write *delayedWarpArg to modify the warp argument passed to the destination.
DEFINE_EVENT(ChangeLevel,
    int16_t          sourceWarpNode;
    struct WarpNode* warpNode;        // destination warp node
    int32_t*         delayedWarpArg;  // mutable warp argument
);

// Fires when the player confirms "Exit Course" from the pause menu.
DEFINE_EVENT(ExitLevel,
    int16_t menuOption; // selected menu option index
);

// ────────────────────────────────────────────────────────────────────────────
// Flying / wing cap (SoH-specific enhancements)
// ────────────────────────────────────────────────────────────────────────────

// Fires when the game decides whether to use the flying triple-jump variant.
// Write *useFlyingVariant = true to force the wing-cap flight animation.
DEFINE_EVENT(SetTripleJumpAction,
    struct MarioState* m;
    bool* useFlyingVariant; // mutable flag; true = use flying variant
);

// Fires each frame in the flying action to check if flight should continue.
// Write *canFly = true to allow flight regardless of cap state.
DEFINE_EVENT(FlyingActionUpdate,
    struct MarioState* m;
    bool* canFly; // mutable flag; true = allow continued flight
);

// Fires when a triple jump launches into wing-cap flight.
// Write *launchVelocity to change the initial upward boost.
DEFINE_EVENT(FlyingTripleJumpLaunch,
    struct MarioState* m;
    f32* launchVelocity; // mutable initial vertical velocity
);
