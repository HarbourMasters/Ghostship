# Level and behavior scripts in archives

Ghostship can load level scripts and behavior scripts from an archive instead of compiling them in. A romhack uses this to replace levels and add behaviors without any code in the port, and a mod can do the same by shipping the files in its o2r.

## How it works

A script is a list of arrays, each a list of commands named after the decomp's macros: `OBJECT_WITH_ACTS`, `WARP_NODE`, `BEGIN`, `CALL_NATIVE` and so on. When the game loads a script it expands every command through the same macro the compiler uses ([ScriptCommands.h](../../src/port/scripts/ScriptCommands.h)), so a loaded script is identical to a compiled one on every platform.

Arguments are one of three things:

| Argument | Example | Becomes |
|---|---|---|
| A number or a constant expression | `0x00010000`, `OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE \| OBJ_FLAG_HOLDABLE`, `MODEL_STAR` | the value |
| A name | `bhvStar`, `bhv_star_door_loop`, `script_func_global_1`, or another array in the same script | a pointer to it |
| A string | `"levels/bob/geo/bob_geo_000488"` | an asset in the archives, like the `__OTR__` strings compiled code uses |

The names and constants the runtime knows are listed in [ScriptSymbols.inc](../../src/port/scripts/generated/ScriptSymbols.inc): every vanilla behavior, every function a vanilla behavior or level script calls, the global level scripts, and the constants from `model_ids.h`, `object_constants.h`, `object_fields.h`, `seq_ids.h`, `dialog_ids.h`, `surface_terrains.h`, `level_commands.h`, `geo_commands.h`, `interaction.h`, `object_list_processor.h` and `shadow.h`, plus level and course numbers. Object fields are their indices, as behavior commands take them. [gen_script_tables.py](../../tools/gen_script_tables.py) writes that list and the command tables; run it after adding a command, behavior or constant to the game.

A name the tables don't have is looked up as a behavior in the archives, `scripts/behaviors/<name>`, and loaded the first time a script uses it. Code mods can add their own names with `Scripts_RegisterPointer` and `Scripts_RegisterConstant` ([Scripts.h](../../src/port/scripts/Scripts.h)).

## Files

| Path in the archive | Resource | Used for |
|---|---|---|
| `scripts/levels/<folder>` | `LevelScript` | Replaces the level whose script is `level_<folder>_entry` (folders as in `levels/level_defines.h`) |
| `scripts/behaviors/<name>` | `BehaviorScript` | A behavior other scripts call `<name>` |
| `textures/skyboxes/levels/<folder>/ptrlist` | 80 skybox tile pointers | The level's skybox, used over the vanilla one its geo layout names |
| `levels/<folder>/<folder>_water_<id>` | `MovtexQuad` | The level's ROM Manager water boxes, geo layout ids `0x5000` to `0x5003` |

The level override is engine-wide ([LevelOverrides.cpp](../../src/port/scripts/LevelOverrides.cpp)): whenever the game is about to run a level script, it checks the archives for `scripts/levels/<folder>` and runs that instead. The romhack loads after the base game and mods after both, so the last one that ships a level wins.

### Text

Text scripts are XML files holding decomp source. The root is `LevelScript` or `BehaviorScript`, and `Entry` names the array the script starts from (by default the array named `*_entry` for levels, the first array for behaviors):

```xml
<BehaviorScript Version="0" Entry="bhvSpinningCoin"><![CDATA[
const BehaviorScript bhvSpinningCoin[] = {
    BEGIN(OBJ_LIST_LEVEL),
    OR_INT(oFlags, OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE),
    BILLBOARD(),
    CALL_NATIVE(bhv_init_room),
    BEGIN_LOOP(),
        CALL_NATIVE(bhv_yellow_coin_loop),
        ADD_INT(oAnimState, 1),
    END_LOOP(),
};
]]></BehaviorScript>
```

Comments, preprocessor lines and `extern` declarations are skipped, so a decomp file pastes in as it is once its assets are written as strings. `#ifdef` blocks aren't evaluated: resolve them first.

### Binary

The binary form is what Torch writes from a ROM ([ScriptFactory.h](../../src/port/importer/ScriptFactory.h)): after the resource header, the array count and the entry name, then each array's name and commands, each command's name, source line and arguments, and each argument as a type byte (0 number, 1 name, 2 asset path) followed by an `s64` or a string.

## Romhacks

A romhack's scripts come from one of two places.

**Text files it ships.** A folder named by `romhack.files` in its recipe, e.g. `scripts` in the hack's folder. The extractor packs every file in it into the romhack's o2r at the same path, without the extension, so `scripts/levels/bob.xml` becomes `scripts/levels/bob`. For ROM Manager hacks with an RM2C export, [rm2c_port.py](../../tools/romhacks/rm2c_port.py) writes these files from the export: each level script and each custom behavior, with asset names turned into archive paths.

**The ROM itself.** Torch extracts `SM64:LEVEL_SCRIPT` and `SM64:BEHAVIOR_SCRIPT` YAML entries in the binary format. A pointer to something Torch extracted becomes its path. A pointer to code becomes a name from the `symbols:` files in the hack's recipe: [assets/symbols/us.yml](../../assets/symbols/us.yml) for the US game, which [gen_us_symbols.py](../../tools/romhacks/gen_us_symbols.py) builds by walking the vanilla US ROM next to the decomp sources, then the hack's own file for code it added, whose functions its code mod registers with `Scripts_RegisterPointer`.

`rom_manager.py yamls --scripts --base-rom` writes, for each level: its script and the scripts it jumps to, the models it loads from the hack's own banks, and every behavior the hack rewrote in place (its bytes differ from vanilla), with the collision and animations those load. Behaviors the hack left alone stay vanilla names. Models and collision that only make sense with another level's banks are left out and reported.
