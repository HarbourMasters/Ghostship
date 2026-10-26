# Romhacks design

This document describes how Ghostship supports existing Super Mario 64 romhacks, so that contributors can port new romhacks in a consistent way. For applying a romhack, see [Playing a romhack](PLAYING.md).

---

Ghostship always runs its own game logic. A romhack brings its assets as an overlay archive, and any custom code it has is ported by hand as a C mod on top of Ghostship's events. A romhack is a special kind of mod: its assets come from the player's own ROM instead of being shipped, and its code mod travels in the same archive.

```
                         ROM SOURCES
     ┌──────────────────┐          ┌──────────────────────────┐
     │  Super Mario 64  │          │  Patched romhack ROM, or │
     │  (base ROM)      │          │  the base ROM + package  │
     └────────┬─────────┘          └────────────┬─────────────┘
              │                                 │
              ▼                                 ▼
   ┌─────────────────────┐        ┌──────────────────────────────┐
   │  First boot         │        │  Settings -> Romhacks        │
   │                     │        │  "Generate Romhack from ROM" │
   │                     │        │  or a hack's "Generate"      │
   └──────────┬──────────┘        └──────────────┬───────────────┘
              │                                  │
              │                       ┌──────────┴───────────┐
              │                       │  Rejected if no      │
              │                       │  recipe package has  │
              │                       │  its SHA1            │
              │                       └──────────┬───────────┘
              ▼                                  ▼
   ┌─────────────────────┐        ┌──────────────────────────────┐
   │  sm64.o2r  (BASE)   │        │  mods/~romhacks/<stem>.o2r   │
   │  - full game        │        │  - changed assets, scripts   │
   │                     │        │  - romhack/ram_* (RAM image) │
   │                     │        │  - its code mod, manifest    │
   └──────────┬──────────┘        └──────────────┬───────────────┘
              │                                  │
              └────────────────┬─────────────────┘
                               ▼
         ┌─────────────────────────────────────────────┐
         │  BOOT                                       │
         │  1. mount sm64.o2r, the romhack, the mods   │
         │  2. build the romhack's code mod with the   │
         │     other code mods -> REGISTER_LISTENER    │
         └─────────────────────────────────────────────┘
```

1. **Base ROM and romhack**: Ghostship distinguishes between the **base ROM**, extracted into `sm64.o2r`, and a **romhack**, an o2r in `mods/~romhacks/` that holds only what the hack changed. Picking a romhack ROM at first boot is refused with *Base Game Required*.

2. **Curated romhacks**: every supported romhack is a folder, `tools/romhacks/<stem>/`, holding everything about it: its recipe (`recipe.yml`, a `config.yml` entry keyed by the SHA1 of the patched ROM, paths relative to the folder), its YAMLs, its symbols and its C mod. `rom_manager.py package <folder>` packs it into a recipe package, which the build ships in `romhacks/` next to the game; players add others to `romhacks/` in the Ghostship folder. A ROM no package (or `config.yml` entry) knows is rejected with *Not a Supported Romhack*. A `recipe.yml`:

   ```yaml
   <patched ROM SHA1>:
     name: <Name>                           # shown in menus and as the saves' identity
     path: yamls                            # YAMLs holding only the changed assets
     symbols: [ assets/symbols/us.yml, symbols/<stem>.yml ]   # shared files keep their repo paths
     romhack:
       id: <Id>
       version: "<version>"
       author: <Author>                     # optional
       description: <one line>              # optional
       website: <download page>             # linked from Supported Romhacks
       disable: [<feature id or group.*>]   # optional, see Feature rules
       force: { <feature id>: <value> }     # optional, see Feature rules
       files: <folder>                      # optional, packed into the o2r as resources
       base:                                # optional, see below
         sha1: <SHA1 of the ROM the patch was made for>
         extend: true                       # ROM Manager hacks: rebuild the sm64extend base first
         patch: <patch file name>
       # code: written by the packer when the folder has a C mod (src/, include/, manifest.json)
     config:
       gbi: F3D
       sort: OFFSET
       gbi_floats: true
       output:
         binary: mods/~romhacks/<stem>.o2r
         code: code/romhacks/<stem>
         headers: include/assets
     segments: [...]                        # optional, the hack's own segment table
   ```

   Ghostship unpacks the matching package into `cache/romhacks/<stem>/` and Torch extracts the entry from there like any other ROM, so `output.binary` is where the archive lands. The extractor ([GameExtractor.cpp](../../src/port/GameExtractor.cpp)) recognizes the entry by its `romhack:` block, creates `mods/~romhacks/`, and adds a `manifest.json`.

   A recipe can also name the ROM its hack was made from and the hack's patch, under `romhack.base` (`sha1`, `extend`, `patch`). When the patch file is next to the recipe (or at the repo root), the packer adds it to the package with an entry for that base ROM whose Torch `preprocess:` jobs rebuild the hack: `sm64extend` (the extended base ROM Manager hacks are built on), then `patch` (PPF3, IPS or BPS, from the package), checking the result's SHA1 against the hack's and restarting with its entry. Settings → Romhacks then shows **Generate** next to the hack, which takes the player's base ROM (`--extract-romhack <rom> --recipe <stem>` on the command line). A vanilla ROM is only treated as a hack's base when that hack is asked for, so first boot still takes it as the base game. The patch is the hack author's work: a package that carries it should only be shared where the author allows it, and patch files are ignored by git.

3. **Identity**: the `manifest.json` written into the romhack o2r carries everything the runtime needs: `name`, `version`, `author`, `description`, `website`, and a `romhack` block with the `id` and the ROM's `rom_sha1`. The runtime never reads `config.yml`, so romhacks work on builds without Torch. When the recipe has `code`, the manifest also gets that mod's `main`, `code_version` and `api_version`, so the script loader builds the hack's code from its own archive; the Mods menu leaves the active romhack out either way.

4. **Loading order**: the base game loads first, then the selected romhack, then mods ([Engine.cpp](../../src/port/Engine.cpp), `LoadResourceFiles`). Resources are looked up by path, so the romhack shadows the base game and mods shadow both. The mod scan skips folders starting with `~`. Only one romhack is active at a time; it's stored in `gSettings.Romhack.Selected`, and the `-hack <stem>` launch argument overrides it for a single launch. Switching needs a restart.

5. **Game data**: numbers, stars, paths and switches compiled into Ghostship are read through events where the game uses them (`GameValue`, `GameFloat`, `SpawnFixedStar`, `ModifyTrajectory`, ... in [GameEvent.h](../../src/port/events/list/GameEvent.h)), so a hack's code mod changes them by listening, like any mod. ROM Manager hacks get `romhack/ram_<address>` blobs, their patched code and data segments and the blocks their code DMAs in, so the mod reads the hack's values from the player's ROM instead of repeating them.

6. **Scripts**: level scripts and behaviors that only arrange what the game already has (objects, warps, vanilla code with other models or collision) ship as script files in the o2r, not as code ([SCRIPTS.md](SCRIPTS.md)). `scripts/levels/<folder>` replaces that level for any archive, romhack or mod.

7. **Custom code**: what scripts can't express, like new native behavior functions or changed game rules, is a C mod in the hack's own archive (the recipe's `code`), built by the script loader like any other code mod. It goes through events (`REGISTER_LISTENER`) and Ghostship's APIs, and makes its functions usable from scripts with `Scripts_RegisterPointer`. Nothing hack-specific is compiled into Ghostship. A hack without code still gets its assets and scripts.

8. **Saves**: each romhack has its own saves folder, `saves/~romhacks/<stem>/` ([Saves.cpp](../../src/port/data/Saves.cpp)). The legacy `default.sav` migration only runs for the base game, so a romhack never takes over the base game's old save.

9. **Romhack Menu**: romhacks are managed in **Settings → Romhacks**, apart from the Mods menu ([RomhackMenuWindow.cpp](../../src/port/ui/RomhackMenuWindow.cpp)). It holds the *Generate Romhack from ROM* button (for a patched ROM), the *Installed Romhacks* list (one enabled at a time) and the *Supported Romhacks* list read from `config.yml` and the recipe packages, with download links and, for hacks whose package carries their patch, a *Generate* button that takes the base ROM.

> [!NOTE]
> A folder name starting with `~` is reserved and managed by the port.

### Layers

A romhack is split into four layers, each with its own path into the port:

| Layer | Where it comes from | Example |
|---|---|---|
| **Assets**: textures, models, geo layouts, collision, dialog, music | delta YAMLs -> romhack o2r | a hack's new textures and level geometry |
| **Game data** compiled into Ghostship | game data events, answered by the hack's code mod from its `romhack/ram_*` blobs | star requirements, boss star spots, paths, camera tuning |
| **Scripts**: level scripts, behaviors built from vanilla code | extracted from the ROM (`rom_manager.py yamls --scripts`), or script files in `romhack.files` -> romhack o2r | a hack's levels, objects and warps; custom behaviors |
| **Custom code**: new native functions, changed rules | the C mod in the hack's folder -> romhack o2r, through events | new object logic, hard modes |

### The romhack's code mod

A hack's code is an ordinary C mod in its folder (`src/`, `include/`, `manifest.json`, as in the mod template, [workspace/template](../../workspace/template)). The packer builds it the way the template's CMake does and adds it to the package as `code/`; the extractor packs that into the romhack o2r and copies its manifest's `main` and versions into the romhack's manifest:

```c
// tools/romhacks/my_hack/src/my_hack.c
#include "mod.h"
#include "port/events/Events.h"
#include "port/scripts/Scripts.h"

static void bhv_my_hack_platform_loop(void) { /* ... */ }

static void on_game_value(IEvent* event) {
    GameValue* ev = (GameValue*) event;
    if (ev->id == GAME_VALUE_STARTING_LIVES) {
        *ev->value = 3;
    }
}

MOD_INIT() {
    // The hack's behavior scripts can now CALL_NATIVE(bhv_my_hack_platform_loop).
    Scripts_RegisterPointer("bhv_my_hack_platform_loop", (const void*) bhv_my_hack_platform_loop);
    REGISTER_LISTENER(GameValue, EVENT_PRIORITY_NORMAL, on_game_value);
}

MOD_EXIT() {
}
```

It runs once per launch, and since switching romhacks needs a restart, listeners never need unregistering. Engine C code can check `Romhack_IsActive()`, `Romhack_GetId()`, `Romhack_GetName()` and `Romhack_GetVersion()` from `port/romhack/Romhack.h`.

### SM64 romhacks

> [!WARNING]
> This section is incomplete, and details differ between hacks made with SM64 Editor, ROM Manager, Quad64 or the decomp.

Most Super Mario 64 romhacks are made with ROM Manager or SM64 Editor, which extend the ROM to 64 MB and relocate level data, so a byte-for-byte diff against the base ROM makes the whole game look rewritten:

- **Segments** are usually moved into the extended region (from ROM `0x800000` upward). The recipe's `segments:` table must match the hack's, not the base game's.
- **Level data** (geometry, collision, geo layouts, scripts) is recompressed or stored raw. It often matches vanilla once decompressed; only the levels the hack actually changed need YAMLs.
- **Behaviors** written as custom assembly can't be extracted; they are ported by hand into the hack's code mod.
- **Music** is usually new sequences and sound banks, which extract like the base game's once their offsets are known.

Decomp-based hacks ship a patch to the decomp source instead. Their assets build like the base game's, and their code changes become the hack's code mod.

#### ROM Manager hacks

[rom_manager.py](../../tools/romhacks/rom_manager.py) handles hacks made with ROM Manager, which all share one layout:

- **Extended base**: the hack is built on top of an sm64extend ROM, and its patch usually only stores the bytes that differ from that ROM. Applying the patch to a plain 8 MB ROM gives a ROM that looks right but has broken bytes all over the extended region. `rom_manager.py extend` rebuilds that base exactly (sm64extend and M64 ROM Extender 1.3b with their defaults), and `rom_manager.py ppf` applies PPF3 patches; Torch's `sm64extend` and `patch` preprocess jobs do the same when a package carries the patch. The recipe's SHA1 is the SHA1 of the extended, patched ROM.
- **Levels**: each level the hack replaced has its own block, with its script in segment 0x19, its area geometry in segment 0x0E and its texture bank in segment 0x09. Texture banks are MIO0 blocks whose data is stored uncompressed, so the YAMLs map segment 0x09 straight to the data.
- **Title logo**: the intro keeps its vanilla script, compiled in, and the hack's logo comes in under the vanilla resource names: `levels/intro/geo/intro_geo_0002D0` if the hack changed that geo layout, and the logo display list and scale tables in `levels/intro` if it replaced the intro's model segment.
- **Skyboxes**: each level loads its own skybox into segment 0x0A, and hacks reuse background IDs for different images or add IDs past the vanilla ten. `rom_manager.py yamls` writes each level's skybox to `textures/skyboxes/levels/<folder>/ptrlist`, which the game uses for that level in place of the vanilla table.
- **YAMLs**: `rom_manager.py yamls` follows the level scripts and writes one YAML per level with its area geo layouts, collision, lights and textures, plus one per texture bank. Torch generates the display lists and vertices from the geo layouts, but the textures and lights have to be listed, so the tool walks the display lists to find them. `--extra` adds assets the level scripts don't point at, such as collision that custom behaviors load.

Level scripts and behaviors are loaded from the archives, not compiled in ([SCRIPTS.md](SCRIPTS.md)). `rom_manager.py yamls --scripts --base-rom` extracts them from the ROM together with the models and behaviors the hack changed. Code the hack added in assembly is ported by hand into its code mod and named in its symbols file (the recipe's `symbols:`), so the extracted scripts call it.

Each hack's folder under [tools/romhacks](../../tools/romhacks) holds its recipe, YAMLs, symbols (its assembly functions named in `symbols/<name>.yml`) and C mod, with how to regenerate it in its README.

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
- **Rules** (`ModRules::Resolve`, called right after the mods are mounted and ordered, before `ShipInit`): the sources, in load order, are the built-in rules, the romhack, then each mounted mod.

A romhack's `disable` and `force` live in its recipe's `romhack:` block and are written into its manifest when it is extracted.

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
