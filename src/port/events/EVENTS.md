# Ghostship Event Reference

Events are dispatched via `CALL_EVENT` or `CALL_CANCELLABLE_EVENT` and consumed via `REGISTER_LISTENER`.

**Cancellable events** — a listener can set `event->Cancelled = true` to block the engine's default behaviour. The exact effect of cancellation is described per event. Non-cancellable events carry data for observation only.

**Mutable pointer fields** — fields typed as a pointer (e.g. `u32* action`) let listeners write through them to change what the engine uses. You do not need to cancel the event to redirect a value; just write to `*ev->field`.

```cpp
REGISTER_LISTENER(EventName, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
    EventName* ev = (EventName*)event;
    // read / modify / cancel
    event->Cancelled = true;
});
```

---

## Engine Events

### `EngineReady`
Fires once at the end of `GameEngine::FinishInit()`, after all mod scripts are loaded. Use for one-time setup that requires the engine to be fully live.

---

### `GameLoopTick`
Fires every main-loop iteration, before game logic runs. Suitable for polling external state at the game's native tick rate.

---

### `GameFrameUpdate`
Fires once per game logic frame inside `level_update`. Runs after `GameLoopTick`. Use for per-frame game-state hooks.

---

### `GameReadInput` _(cancellable)_
Fires before controller inputs are read and processed.  
**Cancel** — skips reading all controllers this frame (useful for cutscene locks or input replay).

---

### `RenderGamePre`
Fires before the game's main render pass begins.

---

### `RenderGamePost`
Fires after the game's main render pass completes.

---

### `RenderHud` _(cancellable)_
Fires inside the HUD render call.  
**Cancel** — suppresses the entire HUD for this frame.

---

### `RenderTextLabels` _(cancellable)_
Fires inside the text-label render call.  
**Cancel** — suppresses all in-world text labels for this frame.

---

### `RenderPauseCourseOptions`
```c
bool* render;
```
Fires inside the pause menu to decide whether the "Exit Course / Continue" panel is shown.  
Write `*render = true` to force the panel visible regardless of the current course.

---

### `GeoLayoutCallASM`  _(cancellable)_
```c
GraphNodeFunc* func;
int16_t*       parameter;
```
Fires when the geo layout engine registers a `node_generated` or `node_background` node — both the original C path and the OTR C++ path.  
Write through `*func` to redirect the callback. Write `*parameter` to change the node parameter. **Cancel** to suppress registering the node entirely.

---

### `BehaviorCallNative`
```c
void (*function)(void);
```
Fires when a behavior script executes a native C function call. Informational — use `func` to identify which behavior is running.

---

### `LevelScriptExecute`
```c
u8 command;
```
Fires for every level script command that is dispatched. `command` is the raw opcode byte.

---

### `LevelScriptCallLoop`
```c
LevelScriptFunction* func;
int16_t*             arg;
```
Fires inside the level script loop handler. Write through `*func` or `*arg` to redirect the loop call.

---

### `LevelScriptBeginArea`
```c
uint8_t* areaIndex;
void**   geoLayoutAddr;
```
Fires when the level script opens a new area. Write through `*geoLayoutAddr` to substitute a custom geo layout.

---

### `LevelInitFromSaveFile`
Fires when the game initialises a level from the save file (e.g. after loading a file or returning from a course). Runs before Mario is placed.

---

### `OnGameFileLoad`
```c
s32 fileNum;
```
Fires when a save file slot is loaded. `fileNum` is 0-based.

---

### `OnGameFileSave`
```c
s32 fileNum;
```
Fires immediately after save data is flushed to disk. `fileNum` is 0-based.

---

### `EntityDistanceLoad` _(cancellable)_
```c
bool* visible;
```
Fires when the engine's distance check would cull an entity from loading.  
Write `*visible = true` or **cancel** to force the entity to load regardless of distance.

---

### `EntityDistanceRender` _(cancellable)_
```c
bool* visible;
```
Fires when the engine's distance check would cull an already-loaded entity from rendering.  
Write `*visible = true` or **cancel** to force the entity to render regardless of distance.

---

### `CameraUpdate`
```c
struct Camera* c;
```
Fires at the very end of `update_camera()` every frame, after Lakitu state is resolved. Safe to read or adjust the final camera position/focus.

---

### `CutsceneStart`
```c
struct Camera* c;
s16            cutsceneId;
```
Fires once when `c->cutscene` transitions from 0 to a non-zero ID. `cutsceneId` is the new cutscene constant (e.g. `CUTSCENE_STAR_SPAWN`).

---

### `CutsceneEnd`
```c
s16 cutsceneId;
```
Fires once when `c->cutscene` returns to 0. `cutsceneId` is the ID of the cutscene that just ended.

---

## Player Events

### `PlayerSetAction` _(cancellable)_
```c
struct MarioState* m;
u32                action;
u32                arg;
```
Fires every time `set_mario_action` is called. `action` is the proposed action constant; `arg` is the secondary argument.  
**Cancel** — blocks the action change; Mario keeps his current action.

---

### `PlayerExecuteAction` _(cancellable)_
```c
s32* result;
```
Fires every frame when the engine executes Mario's current action function.  
Write `*result` to override the return value. **Cancel** — skips executing the action function entirely.

---

### `PlayerCheckCommonAirborneCancels` _(cancellable)_
```c
struct MarioState* m;
s32*               result;
```
Fires inside the airborne common-cancel check (ground pound, ledge grab, etc.).  
Write `*result` to force a specific cancel result. **Cancel** — skips the standard cancel checks.

---

### `PlayerHealthChange` _(cancellable)_
```c
struct MarioState* m;
s32                health;
```
Fires before any health delta is applied. `health` is the change amount (negative = damage, positive = heal).  
**Cancel** — blocks the health change; current HP is preserved.

---

### `PlayerLivesChange` _(cancellable)_
```c
struct MarioState* m;
s32                lives;
```
Fires before a lives delta is applied. `lives` is the change amount (-1 for death, +1 for 1-Up).  
**Cancel** — blocks the lives change.

---

### `PlayerDeath` _(cancellable)_
```c
struct MarioState* m;
PlayerDeathType    type;
```
Fires for every death trigger. `type` describes the cause:

| Constant | Cause |
|---|---|
| `DEATH_TYPE_DEFAULT` | Generic / fall off stage |
| `DEATH_TYPE_FALL` | Fall damage from height |
| `DEATH_TYPE_LAVA` | Lava |
| `DEATH_TYPE_FIRE` | Fire |
| `DEATH_TYPE_QUICKSAND` | Quicksand |
| `DEATH_TYPE_EATEN` | Eaten by enemy |
| `DEATH_TYPE_SQUISHED` | Squished |
| `DEATH_TYPE_DROWNING` | Drowning |
| `DEATH_TYPE_WHIRLPOOL` | Whirlpool |
| `DEATH_TYPE_OUT_OF_BOUNDS` | Fell out of bounds |

**Cancel** — prevents the death sequence from starting.

---

### `PlayerStartedDialog`
```c
struct MarioState* m;
int32_t            dialogId;
```
Fires when Mario enters a dialog interaction. `dialogId` indexes into the dialog table.

---

### `PlayerLanded`
```c
struct MarioState* m;
f32                fallHeight;
```
Fires when Mario lands on a surface after being airborne. `fallHeight` is the height fallen in units (not meters) — the same value used internally for fall damage checks.

---

### `PlayerHit`
```c
struct MarioState* m;
struct Object*     source;
s32                damage;
```
Fires after `m->hurtCounter` is incremented when Mario takes damage from a hazard object. `damage` is the raw damage value before the `×4` multiplier.

---

### `CannonEntered` _(cancellable)_
```c
struct MarioState* m;
struct Object*     cannon;
```
Fires when Mario enters a cannon base, after state is set up but before `ACT_IN_CANNON`.  
**Cancel** — keeps Mario's current action; cannon is not entered.

---

### `PoleGrabbed` _(cancellable)_
```c
struct MarioState* m;
struct Object*     pole;
u32*               lowSpeed;
```
Fires when Mario first grabs a pole (not on every frame while riding). Write `*lowSpeed` to force slow (`ACT_GRAB_POLE_SLOW`) or fast (`ACT_GRAB_POLE_FAST`) grab. **Cancel** — no grab; Mario passes through.

---

### `HootGrabbed` _(cancellable)_
```c
struct MarioState* m;
struct Object*     hoot;
```
Fires when Mario grabs onto Hoot the owl, before `ACT_RIDING_HOOT`.  
**Cancel** — prevents the ride.

---

### `ShellMounted` _(cancellable)_
```c
struct MarioState* m;
struct Object*     shell;
```
Fires when Mario mounts a Koopa shell, after sounds play but before `ACT_RIDING_SHELL_GROUND`.  
**Cancel** — prevents mounting; shell stays on the ground.

---

### `FlameHit` _(cancellable)_
```c
struct MarioState* m;
struct Object*     flame;
u32*               burningAction;  // ACT_BURNING_JUMP or ACT_BURNING_FALL
```
Fires when a non-immune Mario is ignited. Write `*burningAction` to change the burning animation. **Cancel** — suppresses the burn entirely.

---

### `ShockHit` _(cancellable)_
```c
struct MarioState* m;
struct Object*     shock;
```
Fires when a non-immune Mario is shocked. Wraps the full shock response including `take_damage_from_interact_object`.  
**Cancel** — suppresses all shock damage and action change.

---

### `BreakableHit` _(cancellable)_
```c
struct MarioState* m;
struct Object*     breakable;
u32*               interaction;  // INT_HIT_FROM_ABOVE / INT_HIT_FROM_BELOW etc.
```
Fires when Mario attacks a breakable object with sufficient force. Write `*interaction` to change attack type. **Cancel** — suppresses the hit; object is not broken.

---

### `PlayerKnockback` _(cancellable)_
```c
struct MarioState* m;
u32*               action;
```
Fires at the end of `determine_knockback_action`, after the direction and terrain have been resolved.  
Write `*action` to substitute a different action constant. **Cancel** — keeps Mario's current action (no knockback).

---

### `ItemCollected` _(cancellable)_
```c
int16_t            type;         // TYPE_COIN or TYPE_STAR
struct MarioState* marioState;
struct Object*     object;
```
Fires when Mario collects a coin or star. `type` is `TYPE_COIN` or `TYPE_STAR`.  
**Cancel** — suppresses the collection (coin/star is not counted or removed).

---

### `ExitLevel`
```c
int16_t menuOption;
```
Fires when the player confirms "Exit Course" from the pause menu. `menuOption` is the selected index.

---

### `ChangeLevel`
```c
int16_t          sourceWarpNode;
struct WarpNode* warpNode;
int32_t*         delayedWarpArg;
```
Fires when a warp triggers a full level change. `warpNode` describes the destination. Write `*delayedWarpArg` to modify the warp argument passed to the destination.

---

## Rando / Object Events

### `SpawnStar`
```c
int16_t* model;
f32      posX;
f32      posY;
f32      posZ;
```
Fires when a star object is spawned. Write `*model` to substitute a different model.

---

### `SpawnCoinStar` _(cancellable)_
```c
int16_t posX;
int16_t posY;
int16_t posZ;
```
Fires when a coin-star collectible is about to appear. **Cancel** — suppresses spawning it.

---

### `ModifyDefaultStar` _(cancellable)_
```c
f32     posX;
f32     posY;
f32     posZ;
int32_t param;
```
Fires for each star placed by the default star-spawn behavior. **Cancel** — suppresses this star placement.

---

### `ModifyObjectBehavior` _(cancellable)_
```c
struct Object* object;
int16_t        model;
```
Fires when an object's behavior initialises its model. Write through `object` fields or **cancel** to redirect to a custom behavior.

---

### `ModifyRedCoinCount` _(cancellable)_
```c
int8_t* redCoinsCollected;
```
Fires inside the red-coin star logic before the count is checked. Write `*redCoinsCollected` to override the total.

---

### `ModifyObjectVisibility`
```c
struct Object* object;
```
Fires when object visibility is being determined.

---

### `MacroObjectOverride` _(cancellable)_
```c
int16_t model;
int16_t posX;
int16_t posY;
int16_t posZ;
```
Fires during macro object table processing. **Cancel** — skips placing this macro object.

---

### `ObjectSpawned`
```c
struct Object* object;
```
Fires at the end of `create_object()` for every newly allocated game object, before any behavior runs.

---

### `ObjectDestroyed`
```c
struct Object* object;
```
Fires inside `mark_obj_for_deletion()` before `activeFlags` is cleared. The object is still fully valid at this point.

---

## Game Events

### `PaintingEntered`
```c
struct Painting* painting;
```
Fires when Mario enters a painting (transition to `PAINTING_ENTERED` state), before the ripple effect is committed. Use to detect which painting was entered and trigger side effects.

---

### `StarCollected`
```c
s32 courseNum;  // COURSE_NUM_TO_INDEX result (0-based)
s32 starIndex;  // star slot within the course (0-based)
```
Fires only when a genuinely new star is saved — already-owned stars do not trigger this.

---

### `CapSwitchActivated` _(cancellable)_
```c
CapSwitchType type;  // CAP_SWITCH_WING / CAP_SWITCH_METAL / CAP_SWITCH_VANISH
```
Fires when Mario presses a cap switch. **Cancel** — suppresses activating the switch.

---

### `ChainChompRelease` _(cancellable)_
```c
struct Object* entity;
```
Fires when the chain chomp breaks its post and is freed. **Cancel** — keeps the chomp chained.

---

### `BossDefeated` _(cancellable)_
```c
struct Object* entity;
BossType       type;
```
Fires when a boss's defeat condition is met. `type` identifies which boss. **Cancel** — prevents the defeat sequence (boss stays alive).

Available `BossType` values: `BOSS_TYPE_KING_BOBOMB`, `BOSS_TYPE_KING_WHOMP`, `BOSS_TYPE_BIG_BOO_HUNT`, `BOSS_TYPE_BIG_BOO_MERRY_GO_ROUND`, `BOSS_TYPE_BIG_BOO_BALCONY`, `BOSS_TYPE_BIG_BULLY`, `BOSS_TYPE_EYEROK`, `BOSS_TYPE_WIGGLER`, `BOSS_TYPE_CHILL_BULLY`, `BOSS_TYPE_MR_I`, `BOSS_TYPE_BOWSER_BITDW`, `BOSS_TYPE_BOWSER_BITFS`, `BOSS_TYPE_BOWSER_BITS`.

---

### `BossBattleStarted`
```c
BossBattleType type;  // BOSS_BATTLE_KOOPA / BOSS_BATTLE_KOOPA_FINAL / BOSS_BATTLE_GENERIC
```
Fires when a boss battle begins.

---

### `BossBattleEnded`
Fires when any boss battle ends (after defeat sequence or abort).

---

### `SpawnCollectible` _(cancellable)_
```c
struct Object*  entity;
CollectibleType itemType;  // COLLECTIBLE_TYPE_GRAND_STAR / COLLECTIBLE_TYPE_KEY
```
Fires when a grand star or castle key is about to spawn after a Bowser fight. **Cancel** — suppresses spawning it.

---

### `MusicChanged`
```c
s16 seqId;
```
Fires when the background music sequence changes. `seqId` is the new sequence ID.

---

### `WarpStart`
```c
s16 destLevel;
s16 destArea;
s16 warpNode;
```
Fires at the end of `initiate_warp()` after the `sWarpDest` struct is fully populated. The warp is queued but not yet executed.

---

### `WarpEnd`
```c
s16 level;
s16 area;
```
Fires inside `init_mario_after_warp()` after the destination area resets, just before `sWarpDest` is cleared. `level` and `area` are `gCurrLevelNum` / `gCurrAreaIndex` at the moment of arrival.

---

### `ButtonPressed`
```c
struct Controller* controller;
u16                button;
```
Fires per controller per frame when one or more new buttons are pressed (rising edge only — held buttons do not repeat). `button` is a bitmask of the newly pressed buttons (e.g. `A_BUTTON | B_BUTTON`).

---

### `GameEnded`
Fires when the ending credits complete and the game is considered finished.

---

## Audio Events

Audio events are documented inline in [list/AudioEvent.h](list/AudioEvent.h). Summary:

| Event | Cancellable | Thread | Purpose |
|---|---|---|---|
| `PlayMusicEvent` | yes | game | Before a BGM sequence is queued; redirect via `*seqArgs` |
| `StopMusicEvent` | yes | game | Before BGM is stopped; cancel to keep playing |
| `FadeoutMusicEvent` | yes | game | Before BGM fadeout; cancel to suppress |
| `PlaySecondaryMusicEvent` | yes | game | Before ENV player starts; redirect volume/sequence |
| `PlaySfxEvent` | yes | game | Before SFX is enqueued; redirect via `*soundBits` or cancel |
| `PlayDialogSoundEvent` | yes | game | Before dialog SFX plays; redirect via `*dialogID` |
| `AudioUpdateEvent` | no | game | End of `audio_signal_game_loop_tick`; per-frame audio work |
| `SeqLayerPreNoteEvent` | no | **audio** | Before note allocated; swap `layer->sound` for custom PCM |
| `SeqLayerPostNoteEvent` | no | **audio** | After note allocated; pin delay/gateDelay |

> **Warning** — `SeqLayerPreNoteEvent` and `SeqLayerPostNoteEvent` fire on the audio thread. Do not call any game-thread APIs or take game-thread locks inside those listeners.

---

## Render Events (`list/RenderEvent.h`)

### HUD Elements _(all cancellable)_

| Event | Field | Description |
|---|---|---|
| `RenderHudLives` | `s16 lives` | Cancel to suppress the lives counter |
| `RenderHudCoins` | `s16 coins` | Cancel to suppress the coin counter |
| `RenderHudStars` | `s16 stars` | Cancel to suppress the star counter |
| `RenderHudPowerMeter` | `s16 wedges` | Cancel to suppress the health meter |
| `RenderHudCameraStatus` | `s16 status` | Cancel to suppress the camera mode icon |
| `RenderHudTimer` | `u16 timer` | Cancel to suppress the slide timer |

---

### `SkyboxRender` _(cancellable)_
```c
s8* background;
```
Fires in `geo_skybox_main` during render. Write `*background` to substitute a different skybox image ID. **Cancel** — suppresses skybox (black sky).

---

### `MovingTextureRender` _(cancellable)_
```c
s16* movtexId;
```
Fires in `geo_movtex_draw_nocolor` and `geo_movtex_draw_colored` during render. Write `*movtexId` to redirect to a different moving texture group. **Cancel** — suppresses the water/lava/sand surface.

---

### `ScreenTransitionFadeOut` _(cancellable)_
```c
struct WarpTransitionData* transData;
u8 alpha;
```
Fires before a color fade-out overlay is drawn. Write through `transData->red/green/blue` to change color. **Cancel** — suppresses the overlay.

---

### `ScreenTransitionFadeIn` _(cancellable)_
```c
struct WarpTransitionData* transData;
u8 alpha;
```
Fires before a color fade-in overlay is drawn. **Cancel** — suppresses the overlay.

---

### `ScreenTransitionTexture` _(cancellable)_
```c
struct WarpTransitionData* transData;
s8* texId;
```
Fires before a textured wipe transition (star, circle, Mario, Bowser). Write `*texId` to substitute a different wipe texture. **Cancel** — suppresses the wipe entirely.

---

### `FileSelectOverride` _(cancellable)_
Fires in `geo_file_select_strings_and_menu_cursor` during `GEO_CONTEXT_RENDER`, before the file select strings and cursor are printed. **Cancel** — suppresses all default file select rendering; draw your own screen in the listener.

---

### `StarSelectOverride` _(cancellable)_
```c
s16 courseNum;
u8  stars;       // collected star bitmask for this course
s8  selectedAct; // currently highlighted act (0-based)
```
Fires in `geo_act_selector_strings` before act names are printed. **Cancel** — suppresses default act label rendering; draw your own act selector.

---

### `PauseMenuOverride` _(cancellable)_
```c
s16 menuMode;  // MENU_MODE_RENDER_PAUSE_SCREEN or MENU_MODE_UNUSED_0
```
Fires at the entry of `render_pause_courses_and_castle`. **Cancel** — skips all pause menu rendering and returns `MENU_OPT_NONE`; draw your own pause screen.

---

### `CourseCompleteOverride` _(cancellable)_
```c
s16 courseNum;
```
Fires at the entry of `render_course_complete_screen`. **Cancel** — skips the star/coin results screen; return your own.

---

### `DialogOverride` _(cancellable)_
```c
s16 dialogId;
```
Fires at the entry of `render_dialog_entries` when a dialog box is about to render. **Cancel** — suppresses the default dialog box; draw your own text window or cutscene.

---

### `StarSelectRender`
```c
s16 courseNum;
u8  stars;       // bitmask of collected stars in this course
s8  selectedAct; // currently highlighted act (0-based)
```
Fires each frame in `bhv_act_selector_loop`. Use to overlay custom UI on the act selection screen.

---

## New Player Events

### `PlayerCapGained` _(cancellable)_
```c
struct MarioState* m;
struct Object*     source;
u32*               capFlag;   // MARIO_WING_CAP / MARIO_METAL_CAP / MARIO_VANISH_CAP
u16*               capTime;   // duration in frames
```
Fires in `interact_cap` after cap type is resolved. Write `*capFlag` to substitute a different cap or `*capTime` to change duration. **Cancel** — prevents the cap from being applied.

---

### `PlayerObjectGrabbed`
```c
struct MarioState* m;
struct Object*     object;
```
Fires in `mario_grab_used_object` when Mario picks up a holdable object.

---

### `PlayerObjectThrown`
```c
struct MarioState* m;
struct Object*     object;
```
Fires in `mario_throw_held_object` just before the throw trajectory is applied.

---

### `PlayerObjectDropped`
```c
struct MarioState* m;
struct Object*     object;
```
Fires in `mario_drop_held_object` just before the object is released.

---

### `PlayerBounceOnEnemy` _(cancellable)_
```c
struct MarioState* m;
struct Object*     enemy;
u32*               interaction;
```
Fires in `interact_bounce_top` when Mario attacks an enemy with sufficient force. Write `*interaction` to change attack type. **Cancel** — suppresses the bounce and attack entirely.

---

### `PlayerTripleJump`
```c
struct MarioState* m;
```
Fires on the first frame of `act_triple_jump`.

---

### `PlayerWallJump`
```c
struct MarioState* m;
```
Fires on the first frame of `act_wall_kick_air`.

---

### `PlayerLongJump`
```c
struct MarioState* m;
f32*               forwardVel;
```
Fires on the first frame of `act_long_jump`. Write `*forwardVel` to modify jump distance.

---

### `PlayerBackflip`
```c
struct MarioState* m;
```
Fires on the first frame of `act_backflip`.

---

### `PlayerGroundPoundStart`
```c
struct MarioState* m;
```
Fires on the first frame of `act_ground_pound` (when `actionState == 0` and `actionTimer == 0`).

---

### `PlayerGroundPoundLand`
```c
struct MarioState* m;
f32 fallHeight;
```
Fires in `act_ground_pound` the moment Mario lands after the pound. `fallHeight` is `peakHeight - landPos`.

---

### `PlayerWaterEntry`
```c
struct MarioState* m;
```
Fires on the first frame of `act_water_plunge` (Mario enters water).

---

### `PlayerWaterExit`
```c
struct MarioState* m;
```
Fires on the first frame of `act_swimming_end` (Mario breaks the water surface heading to land).

---

### `PlayerFloorTypeChange`
```c
struct MarioState* m;
s16  prevType;
s16* newType;
```
Fires in `perform_ground_step` when `m->floor->type` changes from the previous frame. Write `*newType` to substitute a different surface type for physics/sound purposes.

---

### `PlayerQuicksandSink` _(cancellable)_
```c
struct MarioState* m;
f32*               quicksandDepth;
```
Fires each frame Mario is sinking in quicksand. Write `*quicksandDepth` to control sink depth directly. **Cancel** — prevents all depth changes this frame (stops sinking).

---

### `PlayerWindForce` _(cancellable)_
```c
struct MarioState* m;
f32* pushX;
f32* pushZ;
```
Fires in `mario_update_windy_ground` before velocity is applied. Write `*pushX`/`*pushZ` to change wind force. **Cancel** — suppresses the wind push entirely.

---

## New Game Events

### `PaintingRipple`
```c
struct Painting* painting;
```
Fires when a painting transitions to the `PAINTING_RIPPLE` state (Mario is nearby). Use to detect painting proximity.

---

### `WaterRingPickup` _(cancellable)_
```c
struct MarioState* m;
struct Object*     ring;
s32*               healAmount;
```
Fires in `interact_water_ring`. Write `*healAmount` to change the heal increment. **Cancel** — suppresses the heal entirely.

---

### `TornadoInteraction` _(cancellable)_
```c
struct MarioState* m;
struct Object*     tornado;
```
Fires in `interact_tornado` before Mario is pulled in. **Cancel** — prevents the tornado from affecting Mario.

---

### `WarpDoorInteraction` _(cancellable)_
```c
struct MarioState* m;
struct Object*     door;
u32*               doorAction;  // ACT_PULLING_DOOR / ACT_PUSHING_DOOR / ACT_UNLOCKING_KEY_DOOR
```
Fires in `interact_warp_door` before the door action is set. Write `*doorAction` to redirect. **Cancel** — blocks the door interaction.

---

### `BehaviorTick`
```c
struct Object* object;
```
Fires at the start of `cur_obj_update` for every active object, every frame — before the behavior script executes. Use for per-object per-frame injection. High-frequency event; keep listeners fast.
