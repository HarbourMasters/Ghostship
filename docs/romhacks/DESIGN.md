# Romhacks design

This document describes how Ghostship supports existing Super Mario 64 romhacks, so that contributors can port new romhacks in a consistent way. For applying a romhack, see [Playing a romhack](PLAYING.md).

---

Ghostship always runs its own game logic. A romhack brings its assets as an overlay archive, and any custom code it has is ported by hand on top of Ghostship's events. A romhack is a special kind of mod: its assets come from the player's own patched ROM instead of being shipped, and its code is built into Ghostship.

```
                         ROM SOURCES
     ┌──────────────────┐          ┌──────────────────────────┐
     │  Super Mario 64  │          │  Super Mario 64 +        │
     │  (base ROM)      │          │  romhack patch           │
     └────────┬─────────┘          └────────────┬─────────────┘
              │                                 │
              ▼                                 ▼
   ┌─────────────────────┐        ┌──────────────────────────────┐
   │  First boot         │        │  Settings -> Romhacks        │
   │                     │        │  "Generate Romhack from ROM" │
   └──────────┬──────────┘        └──────────────┬───────────────┘
              │                                  │
              │                       ┌──────────┴───────────┐
              │                       │  Rejected if no      │
              │                       │  config.yml entry    │
              │                       │  for its SHA1        │
              │                       └──────────┬───────────┘
              ▼                                  ▼
   ┌─────────────────────┐        ┌──────────────────────────────┐
   │  sm64.o2r  (BASE)   │        │  mods/~romhacks/<stem>.o2r   │
   │  - full game        │        │  - changed assets only       │
   │                     │        │  - romhack/<table> blobs     │
   │                     │        │  - manifest.json             │
   └──────────┬──────────┘        └──────────────┬───────────────┘
              │                                  │
              └────────────────┬─────────────────┘
                               ▼
         ┌─────────────────────────────────────────────┐
         │  BOOT                                       │
         │  1. mount sm64.o2r, the romhack, the mods   │
         │  2. Port::ApplyDataTables()  (FinishInit)   │
         │  3. Port::Init()  -> REGISTER_LISTENER(...) │
         │     (ShipInit)                              │
         └─────────────────────────────────────────────┘
```

1. **Base ROM and romhack**: Ghostship distinguishes between the **base ROM**, extracted into `sm64.o2r`, and a **romhack**, an o2r in `mods/~romhacks/` that holds only what the hack changed. Picking a romhack ROM at first boot is refused with *Base Game Required*.

2. **Curated romhacks**: every supported romhack has a hand-made recipe in `config.yml`, keyed by the SHA1 of the patched ROM. A ROM without an entry is rejected with *Not a Supported Romhack*. The template is at the end of `config.yml`:

   ```yaml
   <patched ROM SHA1>:
     name: <Name>                           # shown in menus and as the saves' identity
     path: assets/ymls/romhacks/<stem>      # YAMLs holding only the changed assets
     romhack:
       id: <Id>                             # matches src/port/romhack/specific/<Id>.cpp
       version: "<version>"
       author: <Author>                     # optional
       description: <one line>              # optional
       website: <download page>             # linked from Supported Romhacks
       disable: [<feature id or group.*>]   # optional, see Feature rules
       force: { <feature id>: <value> }     # optional, see Feature rules
     config:
       gbi: F3D
       sort: OFFSET
       gbi_floats: true
       output:
         binary: mods/~romhacks/<stem>.o2r
         code: code/romhacks/<stem>
         headers: include/assets
     segments: [...]                        # the hack's own segment table
   ```

   Torch extracts the entry like any other ROM, so `output.binary` is where the archive lands. The extractor ([GameExtractor.cpp](../../src/port/GameExtractor.cpp)) recognizes the entry by its `romhack:` block, creates `mods/~romhacks/`, and adds a `manifest.json`.

3. **Identity**: the `manifest.json` written into the romhack o2r carries everything the runtime needs: `name`, `version`, `author`, `description`, `website`, and a `romhack` block with the `id` and the ROM's `rom_sha1`. The runtime never reads `config.yml`, so romhacks work on builds without Torch. The manifest has no `main`, so the script loader never tries to build it, and the Mods menu leaves the active romhack out.

4. **Loading order**: the base game loads first, then the selected romhack, then mods ([Engine.cpp](../../src/port/Engine.cpp), `LoadResourceFiles`). Resources are looked up by path, so the romhack shadows the base game and mods shadow both. The mod scan skips folders starting with `~`. Only one romhack is active at a time; it's stored in `gSettings.Romhack.Selected`, and the `-hack <stem>` launch argument overrides it for a single launch. Switching needs a restart.

5. **Data tables**: values compiled into Ghostship can't be swapped by an asset. A hack that changes them ships them as `romhack/<table>` blobs in its o2r, and its code port copies the values it trusts over the compiled table in `ApplyDataTables()`. That runs at the end of `FinishInit`'s resource setup, before `ShipInit`, so enhancements that snapshot those tables see the hack's values.

6. **Custom code**: each hack with code has a `Romhack::Port` subclass in `src/port/romhack/specific/<Id>.cpp`, listed in the `ROMHACK_PORT_LIST` X-macro in [RomhackRegistration.cpp](../../src/port/romhack/RomhackRegistration.cpp). Only the active hack's port is created. Custom logic goes through events (`REGISTER_LISTENER`), such as `LevelScriptOverride` for replacing a level's script, and through Ghostship's own APIs. A hack without a port still gets its assets.

7. **Saves**: each romhack has its own saves folder, `saves/~romhacks/<stem>/` ([Saves.cpp](../../src/port/data/Saves.cpp)). The legacy `default.sav` migration only runs for the base game, so a romhack never takes over the base game's old save.

8. **Romhack Menu**: romhacks are managed in **Settings → Romhacks**, apart from the Mods menu ([RomhackMenuWindow.cpp](../../src/port/ui/RomhackMenuWindow.cpp)). It holds the *Generate Romhack from ROM* button, the *Installed Romhacks* list (one enabled at a time) and the *Supported Romhacks* list with download links, read from `config.yml`.

> [!NOTE]
> A folder name starting with `~` is reserved and managed by the port.

### Layers

A romhack is split into three layers, each with its own path into the port:

| Layer | Where it comes from | Example |
|---|---|---|
| **Assets**: textures, models, geo layouts, collision, dialog, music | delta YAMLs -> romhack o2r | a hack's new textures and level geometry |
| **Data tables** compiled into Ghostship | `romhack/<table>` blobs -> the hack's `Port::ApplyDataTables()` | star counts, course names compiled into the game |
| **Custom code**: new behaviors, level scripts, engine changes | `Port::Init()` in `src/port/romhack/specific/<Id>.cpp`, through events | custom object behaviors, replaced level scripts |

### The romhack port interface

```cpp
namespace Romhack {
class Port {
  public:
    // End of FinishInit's resource setup, before ShipInit.
    virtual void ApplyDataTables() {}

    // ShipInit. Register listeners here.
    virtual void Init() {}
};
}
```

A hack's file subclasses `Port` and exposes `Create<Id>Port()`, and `<Id>` goes into `ROMHACK_PORT_LIST`:

```cpp
// src/port/romhack/specific/StarRoad.cpp
#include "port/romhack/RomhackPort.h"
#include "port/events/Events.h"

namespace {
class StarRoadPort : public Romhack::Port {
  public:
    void Init() override {
        REGISTER_LISTENER(LevelScriptOverride, EVENT_PRIORITY_NORMAL, OnLevelScriptOverride);
    }
};
} // namespace

std::unique_ptr<Romhack::Port> CreateStarRoadPort() {
    return std::make_unique<StarRoadPort>();
}

// RomhackRegistration.cpp
#define ROMHACK_PORT_LIST(PORT) PORT(StarRoad)
```

`Init()` runs once per launch, and since switching romhacks needs a restart, listeners never need unregistering. C code can check `Romhack_IsActive()`, `Romhack_GetId()`, `Romhack_GetName()` and `Romhack_GetVersion()` from `port/romhack/Romhack.h`.

### SM64 romhacks

> [!WARNING]
> This section is incomplete, and details differ between hacks made with SM64 Editor, ROM Manager, Quad64 or the decomp.

Most Super Mario 64 romhacks are made with ROM Manager or SM64 Editor, which extend the ROM to 64 MB and relocate level data, so a byte-for-byte diff against the base ROM makes the whole game look rewritten:

- **Segments** are usually moved into the extended region (from ROM `0x800000` upward). The recipe's `segments:` table must match the hack's, not the base game's.
- **Level data** (geometry, collision, geo layouts, scripts) is recompressed or stored raw. It often matches vanilla once decompressed; only the levels the hack actually changed need YAMLs.
- **Behaviors** written as custom assembly can't be extracted; they are ported by hand as code in the hack's `Port`.
- **Music** is usually new sequences and sound banks, which extract like the base game's once their offsets are known.

Decomp-based hacks ship a patch to the decomp source instead (`tools/extract_patch.py` splits one up). Their assets build like the base game's, and their code changes become the hack's `Port`.

## Mod manifests: works_with, disable, force

Ghostship reads three keys from a mod's `manifest.json` on top of the LUS ones (`ModRules`, [ModRules.cpp](../../src/port/ModRules.cpp)). LUS ignores keys it doesn't know, and a mod without them works everywhere and locks nothing.

```json
{
  "name": "Speedrun Practice",
  "version": "1.0",
  "works_with": ["vanilla", { "name": "Star Road", "version": ">=8" }],
  "disable": ["cheats.*", "enhancements.select_all_stars"],
  "force": { "graphics.interpolation_fps": 30 }
}
```

| Key | Meaning |
|---|---|
| `works_with` | Bases the mod runs on: `vanilla`, a romhack's `name`, or `*`. Each entry is a name or `{ "name", "version" }` with a range: comparisons (`>=`, `>`, `<=`, `<`, `=`) separated by spaces or commas, a bare version being a minimum. Missing means any base. |
| `disable` | Feature ids, or `group.*` wildcards, the mod turns off. |
| `force` | `{ feature id or group.*: value }`: toggles and numbers take a number or `true`/`false`. |

- **Mounting** ([ModList.cpp](../../src/port/ModList.cpp)): each mod's manifest is read from its own archive, never the merged file system, where every `manifest.json` shadows the others. A mod whose `works_with` doesn't include the active base isn't mounted; it stays enabled, so it loads again once its base is active, and the Mods menu shows why it didn't load. An archive with a `romhack` block in plain `mods/` isn't mounted either.
- **Rules** (`ModRules::Resolve`, called right after the mods are mounted and ordered, before the data tables and `ShipInit`): the sources, in load order, are the built-in rules, the romhack, then each mounted mod.

A romhack's `disable` and `force` live in its `config.yml` `romhack:` block and are written into its manifest when it is extracted.

## Feature rules

[Features.h](../../src/port/features/Features.h) is the registry of port features a romhack or mod can lock. It knows nothing about mods: `ModRules` hands it the rule sources. Rules are fixed for the session, since changing the romhack or the mods already needs a restart.

- `disable` is the union over every source: reads return the feature's vanilla value and windows show why they are unavailable.
- `force` pins a value. When two sources force different values, the later one in load order wins, the conflict is logged and listed under **Mods › General**. A forced value wins over a disable.
- Built-in rule: the randomizer (`rando.*`) is disabled whenever a romhack is the base, since it shuffles checks the hack replaced.
- `settings.*` (audio, controls, menus, the renderer, the romhack and mod selection) is never lockable. Unknown ids are logged and ignored.
- Nothing writes CVars: the player's own value is kept and comes back once the rule is gone.

Code reads a lockable setting through the registry instead of `CVarGet*`:

| API | Use |
|---|---|
| `int32_t Port_FeatureInt(const char* id, int32_t fallback)` (C) | The forced value, else the vanilla value while disabled, else the feature's CVar (`fallback` is the CVar default). |
| `float Port_FeatureFloat(const char* id, float fallback)` (C) | The same for float CVars. |
| `bool Port_FeatureAllowed(const char* id)` (C) | False while any source disables the feature; for windows and actions. |
| `const char* Port_FeatureDisabledBy(const char* id)` (C) | Name of the first source that disables it, or NULL. |
| `Features::IsLocked`, `LockReason`, `ForCVar`, `Conflicts`, `DrawLockedNotice` (C++) | UI: "Disabled by X" / "Set to V by X", the widget-to-feature lookup, the conflict list, the notice a window draws instead of itself. |

The menu greys a locked widget in `Menu::MenuDrawItem`, next to the race lockout: the feature comes from the widget's CVar, or from `.Feature(id)` for a widget without one, and the tooltip names the romhack or mod. The Tab hotkey for alternate assets does nothing while `graphics.alt_assets` is locked, and the Save Editor and Object Viewer draw the lock reason instead of themselves.

To make another setting lockable, add it to `sDefs` in [Features.cpp](../../src/port/features/Features.cpp) with its vanilla value, and change every read of its CVar outside the menus to `Port_FeatureInt`.

| Id | CVar | Vanilla value |
|---|---|---|
| `cheats.infinite_health` | `gCheats.InfiniteHealth` | 0 |
| `cheats.infinite_lives` | `gCheats.InfiniteLives` | 0 |
| `cheats.always_fly_triple_jump` | `gCheats.AlwaysFlyTripleJump` | 0 |
| `cheats.flying_triple_jump_high_launch` | `gCheats.FlyingTripleJumpHighLaunch` | 0 |
| `cheats.pause_exit_whenever` | `gCheats.PauseExitWhenever` | 0 |
| `cheats.play_in_demo` | `gCheats.PlayInDemo` | 0 |
| `enhancements.disable_draw_distance` | `gEnhancements.DisableDrawDistance` | 0 |
| `enhancements.disable_lod` | `gEnhancements.DisableLOD` | 0 |
| `enhancements.disable_lakitu_cutscene` | `gEnhancements.DisableLakituCutscene` | 0 |
| `enhancements.disable_peach_cutscene` | `gEnhancements.DisablePeachCutscene` | 0 |
| `enhancements.select_all_stars` | `gEnhancements.SelectAllStars` | 0 |
| `enhancements.star_no_exit` | `gEnhancements.StarNoExit` | 0 |
| `enhancements.always_show_power_meter` | `gEnhancements.AlwaysShowPowerMeter` | 0 |
| `enhancements.mirrored_world` | `gEnhancements.Modes.MirroredWorld.Mode` | 0 |
| `enhancements.fix_koopa_race_music` | `gEnhancements.FixKoopaRaceMusic` | 0 |
| `enhancements.fix_heave_ho_sinking` | `gEnhancements.FixHeaveHoSinkingThroughFloor` | 0 |
| `enhancements.fix_yoshi_dialogue_typo` | `gEnhancements.FixYoshiDialogueTypo` | 0 |
| `enhancements.fix_file_select_button_tilt` | `gEnhancements.FixFileSelectButtonTilt` | 0 |
| `camera.default_mode` / `camera.alternate_mode` | `gEnhancements.Camera.DefaultMode` / `AlternateMode` | 0 |
| `camera.improved_c_buttons` | `gEnhancements.Camera.ImprovedCButtonCamera` | 0 |
| `camera.horizontal_analog` / `camera.vertical_analog` | `gEnhancements.Camera.HorizontalAnalog` / `VerticalAnalog` | 0 |
| `camera.center_button` | `gEnhancements.Camera.CenterCameraButton` | 0 |
| `graphics.alt_assets` | `gEnhancements.Mods.AlternateAssets` | 0 |
| `graphics.interpolation_fps` | `gSettings.InterpolationFPS` | 30 |
| `graphics.match_refresh_rate` | `gSettings.MatchRefreshRate` | 0 |
| `rando.enabled` | `gRandoSettings.Enabled` | 0 |
| `devtools.debug_level_select` | `gDeveloperTools.DebugMode` | 0 |
| `devtools.better_level_select` | `gDeveloperTools.BetterLevelSelect` | 0 |
| `devtools.save_editor` / `devtools.object_viewer` | windows `SaveEditor` / `ObjectViewer` | window |
