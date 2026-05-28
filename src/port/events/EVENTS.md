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
Fires when the geo layout engine is about to call an ASM (C function) node.  
Write through `*func` to redirect the call. **Cancel** to suppress it entirely.

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
