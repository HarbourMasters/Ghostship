# Scripting System

This document covers the C-based mod scripting system — how archives are structured, how the manifest is parsed, how signing and integrity verification work, and how to write a mod using the event system.

---

## Table of Contents

1. [Overview](#overview)
2. [Archive Format (.o2r)](#archive-format-o2r)
3. [manifest.json Reference](#manifestjson-reference)
4. [Signing and Integrity Verification](#signing-and-integrity-verification)
   - [How Verification Works](#how-verification-works)
   - [Generating a Key Pair](#generating-a-key-pair)
   - [Signing an Archive](#signing-an-archive)
   - [Trust Levels](#trust-levels)
5. [Writing a C Mod](#writing-a-c-mod)
   - [Project Structure](#project-structure)
   - [Entry and Exit Points](#entry-and-exit-points)
   - [The Build Pipeline (Amalgamation)](#the-build-pipeline-amalgamation)
   - [build.gen](#buildgen)
6. [C API Reference](#c-api-reference)
   - [mod.h Macros](#modh-macros)
   - [Memory Allocation](#memory-allocation)
   - [Calling Functions from Other Mods](#calling-functions-from-other-mods)
7. [Event System](#event-system)
   - [Defining Events](#defining-events)
   - [Registering and Unregistering Listeners](#registering-and-unregistering-listeners)
   - [Event Priority](#event-priority)
   - [Cancellable Events](#cancellable-events)
8. [Dependencies Between Mods](#dependencies-between-mods)
9. [Pre-compiled Binaries](#pre-compiled-binaries)
10. [Shaders and Shader Packs](#shaders-and-shader-packs)
    - [How the renderer uses shaders](#how-the-renderer-uses-shaders)
    - [Shader files and the preprocessor](#shader-files-and-the-preprocessor)
    - [The context](#the-context)
    - [OpenGL](#opengl)
    - [Vulkan](#vulkan)
    - [DirectX 11](#directx-11)
    - [Metal](#metal)
    - [Which backend runs where](#which-backend-runs-where)
    - [Post-processing passes](#post-processing-passes)
    - [Material shaders](#material-shaders)
    - [Settings](#settings)
    - [Example: a vignette pass](#example-a-vignette-pass)
    - [When something goes wrong](#when-something-goes-wrong)
11. [Romhacks](#romhacks)
    - [Playing a romhack](#playing-a-romhack)
    - [Mods on a romhack](#mods-on-a-romhack)
    - [Locking port features](#locking-port-features)
    - [Game data events](#game-data-events)
    - [Level and behavior scripts](#level-and-behavior-scripts)
    - [How it works](#how-it-works)
    - [Porting a romhack](#porting-a-romhack)
    - [ROM Manager hacks](#rom-manager-hacks)
12. [Ghostship APIs](#ghostship-apis)
    - [UI API](#ui-api)
    - [Achievements API](#achievements-api)
    - [Permissions API](#permissions-api)
    - [Relay API](#relay-api)

---

## Overview

Mods are packaged as `.o2r` files, which are standard ZIP archives containing game assets, a `manifest.json` metadata file, and optionally C source files or pre-compiled shared libraries. When the engine loads a mod, the scripting system JIT-compiles the C sources using [TCC (Tiny C Compiler)](https://bellard.org/tcc/) or loads the pre-built binary for the current platform, then calls the mod's `ModInit` entry point.

---

## Archive Format (.o2r)

An `.o2r` file is a ZIP archive. Plain `.zip` files are also accepted. The internal layout is:

```
my-mod.o2r
├── manifest.json          # Required: mod metadata
├── build.gen              # Optional: list of C source files to JIT-compile
├── dist/
│   └── mod.c              # Amalgamated C source (referenced by build.gen)
├── binaries/
│   ├── linux_x64.so       # Optional pre-compiled binaries per platform
│   ├── windows_x64.dll
│   └── darwin.dylib
└── <game assets ...>      # Any other resource files the mod needs
```

`manifest.json` and `build.gen` are the only files the loader looks for by name. Everything else is up to the mod author.

---

## manifest.json Reference

The `manifest.json` file lives at the root of the archive. All fields are optional except `name`.

```json
{
    "name": "My Mod",
    "version": "1.0.0",
    "author": "Your Name",
    "description": "A short description of what this mod does.",
    "website": "https://example.com",
    "license": "MIT",
    "icon": "icon.png",
    "code_version": 1,
    "game_version": 4294967295,
    "main": "build.gen",
    "binaries": {
        "linux_x64": "binaries/linux_x64.so",
        "windows_x64": "binaries/windows_x64.dll",
        "darwin": "binaries/darwin.dylib"
    },
    "dependencies": ["other-mod-name"],
    "checksum": "",
    "signature": "",
    "public_key": ""
}
```

### Fields

| Field | Type | Default | Description |
|---|---|---|---|
| `name` | string | **required** | Display name of the mod. |
| `version` | string | `"1.0"` | Mod version string. |
| `author` | string | `"Unknown"` | Author name. Used in keystore trust prompts. |
| `description` | string | `"No description provided."` | Short human-readable description. |
| `website` | string | `""` | Homepage or repository URL. |
| `license` | string | `"All rights reserved"` | License identifier (e.g. `"MIT"`, `"GPL-3.0"`). |
| `icon` | string | `""` | Path inside the archive to an icon image. |
| `code_version` | integer | `1` | Must match the engine's compiled code version. Mismatches prevent loading. |
| `game_version` | integer | `0xFFFFFFFF` | Required game version. `0xFFFFFFFF` means any version is accepted. |
| `main` | string | `""` | Path to the `build.gen` file listing C sources to JIT-compile. |
| `binaries` | object | `{}` | Map of platform key → path to a pre-compiled shared library inside the archive. See [Pre-compiled Binaries](#pre-compiled-binaries) for platform keys. |
| `dependencies` | array of strings | `[]` | Names of other mods this mod depends on. They will be initialized before this mod. |
| `shaders` | object | `{}` | Post-processing passes and material shaders the mod adds. See [Shaders and Shader Packs](#shaders-and-shader-packs). |
| `checksum` | string | `""` | BLAKE2b-512 hex digest injected by `tools/sign.py`. Do not set manually. |
| `signature` | string | `""` | Ed25519 hex signature injected by `tools/sign.py`. Do not set manually. |
| `public_key` | string | `""` | Author's Ed25519 raw public key (hex) injected by `tools/sign.py`. Do not set manually. |

> **Note on `code_version`:** The engine enforces that `code_version` in the manifest matches the version it was compiled against. If they differ, the mod will not be loaded. Increment this value when your mod's compiled interface changes.

---

## Signing and Integrity Verification

### How Verification Works

Archive validation is performed in `Archive::Validate()` and proceeds in two stages:

**1. Checksum verification**

A BLAKE2b-512 digest is computed over all non-manifest files in the archive, processed in sorted path order. Both the file path and the file contents are fed into the hash. The result is compared against the `checksum` field in `manifest.json`. If they do not match, the archive is rejected.

**2. Signature verification**

The raw BLAKE2b digest bytes are verified against the Ed25519 `signature` using the `public_key` from the manifest. The engine checks the public key against its internal **Keystore** (a set of trusted Ed25519 public keys). If the key is not already trusted:

- If an `UntrustedArchiveHandler` callback is registered by the host application, the user is prompted to trust the key. On approval, the key is added to the keystore for future sessions.
- If no handler is registered, the archive is rejected.

System keys (hard-coded trusted publisher keys bundled with the engine) are always accepted without a prompt.

An archive that has no `checksum` field is treated as unsigned. It can still be loaded depending on the configured [Trust Level](#trust-levels), but `IsSigned()` and `IsChecksumValid()` will both return `false`.

### Generating a Key Pair

Use `tools/generate_keys.py` to create an Ed25519 key pair:

```sh
# Install the dependency first
pip install cryptography

# Generate keys (no passphrase)
python tools/generate_keys.py

# The script writes:
#   private_key.pem   — keep this secret
#   public_key.pem    — can be distributed
```

To protect the private key with a passphrase, edit the script and pass a password string to `generate_keypair(password="your-passphrase")`.

### Signing an Archive

Use `tools/sign.py` to sign a built `.o2r` / `.zip`:

```sh
# Sign without passphrase
python tools/sign.py my-mod.o2r private_key.pem

# Sign with passphrase
python tools/sign.py my-mod.o2r private_key.pem
```

The script:
1. Computes a BLAKE2b-512 checksum over all non-manifest files (sorted by path).
2. Signs the raw checksum bytes with the Ed25519 private key.
3. Injects `checksum`, `signature`, and `public_key` (all hex-encoded) into `manifest.json` inside the zip in-place.

You must re-sign the archive any time you change its contents.

### Trust Levels

The engine exposes a `SafeLevel` setting that controls how unsigned or untrusted archives are handled:

| Level | Behavior |
|---|---|
| `DISABLE_SCRIPTS` | No scripts are loaded at all. |
| `ONLY_TRUSTED_SCRIPTS` | Only archives with a valid checksum **and** a known-trusted signature are loaded. Unsigned archives are rejected. |
| `WARN_UNTRUSTED_SCRIPTS` | *(default)* Unsigned archives log a warning but still load. |
| `ALLOW_ALL_SCRIPTS` | All archives load silently regardless of signing status. |

---

## Writing a C Mod

### Project Structure

The recommended layout for a mod project:

```
my-mod/
├── include/
│   └── mod.h              # Core macros (MOD_INIT, MOD_EXIT, HM_API)
├── src/
│   └── main.c             # Your mod logic
├── manifest.json          # Mod metadata
└── CMakeLists.txt         # Build configuration (amalgamation pipeline)
```

After building, the output goes to:

```
output/
├── dist/
│   └── main.c             # Amalgamated single-file output
├── build.gen              # Auto-generated list of output paths
└── manifest.json          # Your manifest (copied here for packaging)
```

Package the contents of `output/` into a zip and rename it to `.o2r`.

### Entry and Exit Points

Every mod must export two functions. Use the macros from `mod.h`:

```c
#include "mod.h"

MOD_INIT() {
    // Called once when the mod is loaded.
    // Register event listeners here.
}

MOD_EXIT() {
    // Called once when the mod is unloaded or the engine shuts down.
    // Unregister listeners and free resources here.
}
```

`MOD_INIT` expands to `HM_API void ModInit` and `MOD_EXIT` to `HM_API void ModExit`. `HM_API` resolves to `__declspec(dllexport)` on Windows and `__attribute__((visibility("default")))` on other platforms, ensuring the dynamic linker can find the symbols.

### The Build Pipeline (Amalgamation)

Because mods are JIT-compiled by TCC at runtime, they must be packaged as self-contained C translation units — no separate object files or link steps are available at runtime.

The build system runs `tools/merge.py` on each `.c` file in `src/`. `merge.py` recursively inlines all `#include` directives that resolve to local header files, producing a single amalgamated `.c` file per source file in `output/dist/`. System headers (`<stdio.h>`, etc.) are left as-is.

**Important:** Because all source files ultimately share a single global namespace when loaded, any helper function that should not be visible outside your file must be declared `static`. Failing to do so risks symbol collisions with the engine or other mods.

### build.gen

`build.gen` is a plain text file listing the paths to amalgamated `.c` files to compile, one per line:

```
dist/main.c
dist/utils.c
```

Paths are relative to the root of the archive. The `main` field in `manifest.json` points to this file.

---

## C API Reference

### mod.h Macros

| Macro / Symbol | Expands To | Description |
|---|---|---|
| `HM_API` | `__declspec(dllexport)` / `__attribute__((visibility("default")))` | Marks a symbol as exported from the shared library. |
| `MOD_INIT()` | `HM_API void ModInit` | Declares the mod initialization entry point. |
| `MOD_EXIT()` | `HM_API void ModExit` | Declares the mod shutdown entry point. |
| `CALL_FUNC(mod, func, ...)` | `((func##Func)ScriptGetFunction(mod, func##Symbol))(...)` | Calls an exported function from another loaded mod by name. |
| `malloc(size)` | `GameEngine_Malloc(size)` | Routes heap allocation through the engine allocator. |
| `free(ptr)` | `GameEngine_Free(ptr)` | Routes heap deallocation through the engine allocator. |

> **Note:** `malloc` and `free` are redefined by `mod.h` to use engine-managed allocators. Do not include `<stdlib.h>` after including `mod.h`, or the redefinition will conflict.

### Memory Allocation

Always use `malloc`/`free` as provided by `mod.h`. These route through `GameEngine_Malloc` and `GameEngine_Free`, ensuring allocations are tracked by the engine and freed correctly when the mod is unloaded.

### Calling Functions from Other Mods

To call an exported function from another mod or from the engine's scripting bridge, use `ScriptGetFunction`:

```c
// Prototype of the function you want to call
typedef void (*MyFuncType)(int value);

// Look up the function from module "other-mod"
MyFuncType fn = (MyFuncType)ScriptGetFunction("other-mod", "MyExportedFunction");
if (fn != NULL) {
    fn(42);
}
```

Or use the `CALL_FUNC` macro if you define the function type and symbol name with the expected naming convention.

---

## Event System

The event system lets mods hook into points in the engine and game loop without modifying source code. Events are identified by a global `EventID` that is registered at startup and dispatched via `EventSystemCallEvent`.

### Defining Events

Events are defined using the `DEFINE_EVENT` macro from `include/ship/events/EventTypes.h`. This generates a struct type and declares its global `EventID`:

```c
// An event with no extra data
DEFINE_EVENT(MySimpleEvent);

// An event that carries additional fields
DEFINE_EVENT(MyDataEvent,
    int someValue;
    float otherValue;
);
```

The first member of every generated struct is always `IEvent Event`, which holds the `Cancelled` flag. When dispatching, use the corresponding `CALL_EVENT` or `CALL_CANCELLABLE_EVENT` macros:

```c
// Fire a simple event
CALL_EVENT(MySimpleEvent);

// Fire a data event
CALL_EVENT(MyDataEvent, .someValue = 42, .otherValue = 3.14f);

// Fire a cancellable event — code after the macro only runs if not cancelled
CALL_CANCELLABLE_EVENT(MyDataEvent, .someValue = 1, .otherValue = 0.0f) {
    // only reached if no listener cancelled the event
}
```

Events must be registered once at engine startup using `REGISTER_EVENT` before they can be dispatched or listened to:

```c
REGISTER_EVENT(MySimpleEvent);
REGISTER_EVENT(MyDataEvent);
```

### Registering and Unregistering Listeners

```c
#include "mod.h"
// Include the header that declares the events you want to listen to
#include "your/events/header.h"

ListenerID gMyListenerID;

void OnMyEvent(IEvent* event) {
    // Cast to the concrete event type to access its fields
    MyDataEvent* e = (MyDataEvent*)event;
    // ...
}

MOD_INIT() {
    gMyListenerID = REGISTER_LISTENER(MyDataEvent, EVENT_PRIORITY_NORMAL, OnMyEvent);
}

MOD_EXIT() {
    UNREGISTER_LISTENER(MyDataEvent, gMyListenerID);
}
```

Always store the returned `ListenerID` globally so you can unregister in `MOD_EXIT`. Failing to unregister will cause a use-after-free when the mod is unloaded.

**Macros:**

| Macro | Description |
|---|---|
| `REGISTER_LISTENER(eventType, priority, callback)` | Registers `callback` for `eventType` at `priority`. Returns a `ListenerID`. |
| `UNREGISTER_LISTENER(eventType, listenerID)` | Removes the listener identified by `listenerID` from `eventType`. |

### Event Priority

Listeners for the same event are called in descending priority order.

| Priority | Description |
|---|---|
| `EVENT_PRIORITY_LOW` | Called after normal-priority listeners. |
| `EVENT_PRIORITY_NORMAL` | Default priority for most use cases. |
| `EVENT_PRIORITY_HIGH` | Called before normal-priority listeners. |

### Cancellable Events

Events fired with `CALL_CANCELLABLE_EVENT` or `CALL_CANCELLABLE_RETURN_EVENT` can be cancelled by any listener. To cancel, set `Cancelled` on the base `IEvent`:

```c
void OnMyEvent(IEvent* event) {
    MyDataEvent* e = (MyDataEvent*)event;
    if (/* some condition */) {
        e->Event.Cancelled = true;
    }
}
```

Once cancelled:
- Remaining lower-priority listeners still receive the event but can check `event->Cancelled`.
- The engine code block gated by `CALL_CANCELLABLE_EVENT` is skipped.
- `CALL_CANCELLABLE_RETURN_EVENT` causes an immediate `return` in the calling function.

---

## Dependencies Between Mods

List the `name` values of mods your mod requires in the `dependencies` array:

```json
{
    "name": "my-mod",
    "dependencies": ["base-library-mod"]
}
```

The scripting system performs a topological sort (Kahn's algorithm) across all loaded mods before calling `ModInit`, ensuring dependencies are always initialized first and uninitialized last.

Circular dependencies are detected and will prevent loading.

---

## Pre-compiled Binaries

If you want to ship a pre-compiled shared library instead of (or in addition to) C sources, populate the `binaries` map with platform keys pointing to paths inside the archive:

```json
{
    "binaries": {
        "linux_x64":     "binaries/linux_x64.so",
        "linux_arm64":   "binaries/linux_arm64.so",
        "windows_x64":   "binaries/windows_x64.dll",
        "windows_arm64": "binaries/windows_arm64.dll",
        "darwin":        "binaries/darwin.dylib",
        "android":       "binaries/android.so",
        "ios":           "binaries/ios.dylib"
    }
}
```

The loader checks `binaries` before attempting JIT compilation. If a matching entry is found for the current platform, the pre-built library is extracted to a temporary file and loaded directly. If no match is found and `main` is set, the C sources are JIT-compiled instead.

### Platform Keys

| Key | Platform |
|---|---|
| `windows_x64` | Windows, x86-64 |
| `windows_arm64` | Windows, ARM64 |
| `darwin` | macOS (any arch) |
| `linux_x64` | Linux, x86-64 |
| `linux_x86` | Linux, x86 (32-bit) |
| `linux_arm64` | Linux, ARM64 / AArch64 |
| `linux_generic` | Linux, other architectures |
| `android` | Android |
| `ios` | iOS |
| `bsd` | BSD variants |

---

## Shaders and Shader Packs

A mod can change how the game is drawn without any code. A **shader pack** lists its shaders under `shaders` in `manifest.json`: **post-processing passes** run full screen on the finished frame, one after another, and **material shaders** replace the shader a display list is drawn with. A mod can also replace one of the built-in shaders, such as `shaders/opengl/default.shader.glsl`, by shipping a file at the same path; the mod with the highest priority wins, as for any file. The built-in shaders, in `libultraship/src/fast/shaders/` and inside `ghostship.o2r`, are the reference for everything below. The Mods list marks shader packs with a paint brush.

```json
{
    "name": "Vignette Pack",
    "shaders": {
        "passes":    [ { "shader": "shaders/vignette/vignette", "enabled": true } ],
        "materials": [ { "dlist": "actors/example/example_dl", "shader": "shaders/vignette/material" } ]
    }
}
```

| Key | Meaning |
|---|---|
| passes[].shader | The pass's shader path in the archive, without an extension. Passes run in mod load order, then list order. |
| passes[].enabled | Whether the pass starts on. Defaults to `true`. |
| materials[].dlist | The archive path of the display list to draw with the shader. A later mod's entry for the same display list wins. |
| materials[].shader | The shader path, without an extension. `"enabled": false` leaves the entry out. |

### How the renderer uses shaders

The N64 draws with a fixed pipeline configured per draw: the color combiner, texture tiles, fog, alpha modes. The renderer turns each distinct configuration into a shader by running a **template** through the prism preprocessor with that configuration as its context, then compiling the result. `default.shader` is that template; a custom shader is a template too, used instead of it for the draws it applies to.

- Every distinct configuration is its own compiled program, cached by the configuration and the custom shader in use. A material shader is therefore compiled once per configuration its display lists use, and a pass once.
- A custom shader gets **exactly the same context** as the default one. Only the file changes.
- Shaders compile at runtime, the first time a configuration is drawn. Changing a `@setting` clears the cache and recompiles everything, since settings are written into the source.
- The texture filter mode is written into shaders too, but changing it doesn't recompile the ones already built.

### Shader files and the preprocessor

The renderer adds the active backend's extension to a shader path:

| Backend | File | Processed | Entry points |
|---|---|---|---|
| OpenGL | `<shader>.glsl` | twice, once per stage | `main` in each stage |
| Vulkan | `<shader>.glsl`, the same file | twice, once per stage | `main` in each stage |
| DirectX 11 | `<shader>.hlsl` | once | `VSMain` and `PSMain` |
| Metal | `<shader>.metal` | once | `vertexShader` and `fragmentShader` |

Ship a file for every backend you support. Put them in the mod's `assets/` folder at the manifest's path (`assets/shaders/vignette/vignette.glsl`); the build copies them in as they are. Shader files are plain text run through **prism**. The first line of every file, and of every file it includes, is a header with a `type`, which prism removes with the blank line after it:

```glsl
@prism(type='fragment', name='Vignette', version='1.0.0', description='Darkens the edges', author='You')
```

| Directive | What it does |
|---|---|
| @{expr} | Writes a value or a function's output: `@{GLSL_VERSION}`, `@{texture}`, `@{append_formula(...)}`. `@{x = expr}` sets a variable and writes nothing. |
| @if(c) … @elseif(c) … @else … @end | Keeps the branch whose condition is exactly 1. `@if(c) text` on a single line needs no `@end`. Untaken branches are skipped, so they may name variables that don't exist in this context. |
| @for(i in 0..2) … @end | Repeats the block for `i` = 0, 1. It can also walk an array from the context. |
| @include("path") | Inserts another file from the archives. It is loaded even in a branch that isn't taken, so it has to exist. |
| @setting(…) | A value players can tweak, see [Settings](#settings). |

- Expressions know `|| && == ! + - * /`, numbers, `"strings"`, `true`/`false`, indexing (`o_clamp[0][1]`) and calls, but not `!=`, `<` or `>`.
- Name your own variables in UPPER_CASE: names starting with lowercase `in`, `if`, `else`, `then`, `true` or `false` are misread.
- Make an `@if`/`@elseif` chain the last thing in its block, or wrap it in an `@else`: when an `@elseif` branch is taken, the rest of the enclosing block is skipped.
- Output lines are trimmed and blank lines removed.

### The context

Every backend fills the context from the draw's configuration. These are the same everywhere (where a backend lacks one, its section says so):

| Variable | Meaning |
|---|---|
| BACKEND | `"opengl"`, `"vulkan"`, `"directx"` or `"metal"`; and `BACKEND_OPENGL`, `BACKEND_VULKAN`, `BACKEND_DIRECTX`, `BACKEND_METAL` as true/false. |
| o_textures[2] | Whether texture tile 0 / 1 is used. |
| o_clamp[2][2] | Whether tile 0 / 1 clamps in S / T. |
| o_palette[2], o_masks[2], o_blend[2] | Whether the tile is a palette (CI) texture, and whether it has mask and blend textures. |
| o_c[2][2][4] | The combiner: for each cycle, color and alpha, its four inputs (`SHADER_0`, `SHADER_INPUT_1`…`7`, `SHADER_TEXEL0`, `SHADER_TEXEL0A`, `SHADER_TEXEL1`, `SHADER_TEXEL1A`, `SHADER_1`, `SHADER_COMBINED`, `SHADER_NOISE`). |
| o_do_single, o_do_multiply, o_do_mix [2][2], o_color_alpha_same[2] | Which shape each combiner formula has, and whether color and alpha are the same, so it can be written shorter. |
| o_inputs | How many combiner inputs (`uInputs`) are used. |
| o_alpha, o_fog, o_grayscale, o_shade | Alpha is used, fog is on, grayscale is on, the shade color is an input. |
| o_texture_edge, o_alpha_threshold, o_invisible, o_noise | Cutout alpha, alpha compare against a threshold, nothing drawn, dither noise. |
| o_2cyc, o_prim_depth | Two-cycle mode, a fixed primitive depth. |
| o_mip_lod, o_uses_lod, o_two_tile_lod | Level of detail: mipmapped, the LOD fraction is used, two tiles blended by LOD. |
| o_lighting, o_point_lighting, o_texgen, o_texgen_linear | Vertex lighting, point lights, environment-mapped texture coordinates. |
| o_three_point_filtering | The N64's three-point texture filter is selected; compare with `FILTER_THREE_POINT`. |
| append_formula(o_c[c], single, mult, mix, with_alpha, only_alpha, alpha, first_cycle) | Writes one combiner formula in the backend's language, as `default.shader` uses it. |
| update_floats(n) | Adds a vertex attribute of `n` floats to the vertex layout (OpenGL's vertex stage, DirectX, Metal). A custom vertex stage must call it like the default one or the vertex stride is wrong; including the backend's `fast3d_vs.glsli` does it for you. |

Vertices are packed in this order: position (4 floats), matrix slot (1), texture coordinate 0 (2, if tile 0 is used), texture coordinate 1 (2, if tile 1 is used), then shade color or normal (3, or 4 with alpha, if `o_shade` or `o_lighting`). Texture coordinates are in texels; the vertex stage applies the UV transform. Shaders flip Y themselves by multiplying clip-space Y by `y_scale.x`.

Besides the draw's own uniforms, every backend has `uCustom[32]`, 32 `vec4`s: `uCustom[0]` is (frame count, seconds since start, seconds since the last frame, 0) and `uCustom[1]` is (framebuffer width, height, 1/width, 1/height). The rest are free for the game.

### OpenGL

| Aspect | Details |
|---|---|
| Compile | GLSL compiled by the driver. The file is processed twice, fragment stage first (`VERTEX_SHADER` false) then vertex stage (`VERTEX_SHADER` true), each with its own context, then linked. |
| Context per stage | The fragment stage has the combiner (`o_c`, `append_formula`, `SHADER_*`, `FILTER_*`), textures, palette, masks and blend; the vertex stage has `update_floats`, `out` and the lighting and texgen flags. Keep stage-only names inside `@if(VERTEX_SHADER)` / `@else`. |
| Inputs | Attributes are found by name: `aVtxPos`, `aMtxSlot`, `aTexCoord0`, `aTexCoord1`, `aShade`. Varyings: `vTexCoord0`, `vTexCoord1`, `vFogFactor`, `vShade`. |
| Uniforms | By name: `uInputs[]`, `uFogColor`, `uGrayscaleColor`, `uUvTransform[2]`, `uTexClamp[2]`, `uFogParams`, `uPaletteParams[2]`, `uLodParams`, `uCustom[32]`, `uMtxPalette[32]`, `uYScale`, lighting (`uAmbient`, `uNumLights`, `uLights`…), `prim_depth`, `lod_max`, `texture_width[2]`, `texture_height[2]`, `texture_filtering[2]`, `frame_count`, `noise_scale`. On GLES, declare `uniform highp vec4 uCustom[32];` in **both** stages or linking fails. Use `uCustom[0]` for time: `frame_count` is only uploaded when a program is bound. |
| Textures | `uTex0` (unit 0), `uTex1` (1), `uTexMask0`/`1` (2, 3), `uTexBlend0`/`1` (4, 5), `uTexPal` (6), bound when the flag is set. |
| Includes | In `shaders/opengl/include/`: `fast3d_vs.glsli`, the whole default vertex stage; `filter.glsli`, `hookTexture2D()`, `filter3point()`, `fromLinear()` and the `WRAP` macros; `noise.glsli`, `random(vec3)`; `palette.glsli`, `paletteSample()`. |
| Gotchas | The framebuffer is RGB without alpha. On GLES, depth is scaled and there is no depth clamp. Compile errors go to the standard error output (a vertex stage error says nothing), link errors to the log; both stop the game. |

**Dialects**: the context spells what differs between GL versions:

| Variable | macOS (4.1 core) | GLES 3 (Android, web) | Desktop |
|---|---|---|---|
| GLSL_VERSION | `#version 410 core` | `#version 300 es` + precision | `#version 130` (fragment), `#version 110` (vertex) |
| attr | `in` | `in` | `varying` (fragment), `attribute` (vertex) |
| out (vertex) | `out` | `out` | `varying` |
| texture (fragment) | `texture` | `texture` | `texture2D` |
| vOutColor (fragment) | `vOutColor` | `vOutColor` | `gl_FragColor` |
| core_opengl, opengles | true, false | false, true | false, false |

On core and ES, declare `out vec4 vOutColor;` yourself (`@if(core_opengl || opengles)`).

### Vulkan

| Aspect | Details |
|---|---|
| Compile | The same `.glsl` as OpenGL, processed once per stage (`VERTEX_SHADER` 1 or 0) and compiled to SPIR-V at runtime with shaderc (Vulkan 1.1). Write `#version 450` on the first line after the header: there is no `GLSL_VERSION` here, nor `attr`, `out`, `texture` or `vOutColor`. |
| Context | One context for both stages, with everything listed above. |
| Inputs | Fixed locations, 0 `aVtxPos`, 1 `aMtxSlot`, 2 `aTexCoord0`, 3 `aTexCoord1`, 4 `aShade`; varyings 0 `vTexCoord0`, 1 `vTexCoord1`, 2 `vFogFactor`, 3 `vShade`. The fragment output is `layout(location = 0) out vec4 vOutColor`. |
| Uniforms | `@include("shaders/vulkan/include/common.glsli")` declares them, in set 0: `frameU` (binding 0: `frame_count`, `noise_scale`), `drawU` (binding 1: `inputs[6]`, `fog_color`, `grayscale_color`, `uv_transform[2]`, `tex_clamp[2]`, `fog_params`, `palette_params[2]`, `lod_params`, the custom registers, …), `lightU` (binding 2), `xformU` (binding 3: `mtx_palette[32]`, `y_scale`). `uCustom` is defined as `drawU.uCustomRegs`. |
| Textures | Set 1: binding 0 `uTex0`, 1 `uTex1`, 2 and 3 the masks, 4 and 5 the blend textures, 6 `uTexPal`. |
| Includes | In `shaders/vulkan/include/`: `common.glsli`, `fast3d_vs.glsli` (the whole vertex stage), `palette.glsli`. The filter and noise helpers are written inside `default.shader.glsl`, not in includes. |
| Errors | A compile error logs the message and prints the generated source with line numbers to the standard error output, then stops the game, so you can see the code prism produced. |

### DirectX 11

| Aspect | Details |
|---|---|
| Compile | The `.hlsl` file is processed once and compiled twice with `D3DCompile`: `VSMain` as `vs_4_0` and `PSMain` as `ps_4_0`. There is no `VERTEX_SHADER`: both stages are in the one file. `o_root_signature` is always false. |
| Inputs | Semantics `POSITION` (float4), `MTXSLOT` (float), `TEXCOORD0`, `TEXCOORD1` (float2), `SHADE` (float3 or float4). Call `@{update_floats(n)}` for each, as the default does inside its `PSInput` struct. Between stages: `SV_POSITION`, `uv0`/`uv1 : TEXCOORDn`, `fogFactor : FOG`, `shade : SHADE`. `PSMain` returns `SV_TARGET`, plus `SV_Depth` for primitive depth. |
| Constant buffers | Pixel stage b0 `PerFrameCB` (`noise_frame`, `noise_scale`), b1 `PerDrawCB`, b2 `PerPrimDepthCB`; vertex stage b1 `PerDrawCB`, b3 `LightCB`, b4 `TransformCB` (`mtx_palette[32]`, `y_scale`). `PerDrawCB` holds `textures[7]`, `combiner_inputs[6]`, `fog_color`, `grayscale_color`, `uv_transform[2]`, `texture_clamp[2]`, `fog_params`, `palette_params[2]`, `lod_params`, `uCustom[32]`, `debug_tint`; declare it with the full layout, in this order, even if you only read part of it. |
| Textures | `g_texture0`/`g_sampler0` (t0/s0), `g_texture1`/`g_sampler1` (t1/s1), masks t2–t3, blend t4–t5, `g_texturePal`/`g_samplerPal` (t6/s6). |
| Includes | None; `default.shader.hlsl` holds everything. |
| Errors | A compile error shows a message box with the compiler's output (on Xbox, the log) and stops the game. |

### Metal

| Aspect | Details |
|---|---|
| Compile | The `.metal` file is processed once and compiled from source at runtime; the functions must be named `vertexShader` and `fragmentShader`. There is no `VERTEX_SHADER`, and of the filter constants only `FILTER_THREE_POINT`. |
| Inputs | Declare each vertex field as `[[attribute(@{get_vertex_index()})]]` followed by `@{update_floats(k)}`, in order: position (4), mtxSlot (1), texCoord0/1 (2), shade (3 or 4). `update_floats` takes 1 to 4 only. |
| Buffers | Vertex stage 0 vertices, 1 `LightUniforms`, 2 `DrawUniforms`, 3 `TransformUniforms`; fragment stage 0 `FrameUniforms` (`frameCount`, `noiseScale`), 1 `DrawUniforms`. `DrawUniforms` holds `textureFiltering[7]`, `prim_depth`, `lod_max`, `inputs[6]`, `fog_color`, `grayscale_color`, `uv_transform[2]`, `texture_clamp[2]`, `fog_params`, `palette_params[2]`, `lod_params`, `uCustom[32]`, `debug_tint`. |
| Textures | `uTex0`/`uTex0Smplr` at texture/sampler 0, `uTex1` at 1, masks at textures 2–3, blend at 4–5, `uTexPal` at 6. The output is `[[color(0)]]`, plus `[[depth(any)]]` for primitive depth. |
| Includes | None; the helpers are inside `default.shader.metal`. |
| Errors | A compile error is logged as "Failed to compile shader library" with Metal's message. The game is likely to crash right after. |

### Which backend runs where

| Platform | Backends |
|---|---|
| Windows | DirectX 11, Vulkan, OpenGL |
| Xbox | DirectX 11 |
| macOS | Metal, Vulkan, OpenGL 4.1 |
| iOS | Metal |
| Linux | Vulkan, OpenGL |
| Android, web | OpenGL ES 3 |

Vulkan is only there in builds made with the Vulkan SDK and shaderc. Players switch in **Settings › Graphics › Renderer API**, which takes effect after a restart. A pack that only ships `.glsl` works on OpenGL and Vulkan, and stops the game on DirectX and Metal.

### Post-processing passes

A pass draws the whole screen once, at the game's resolution, reading the image the previous pass drew (the game's frame, after resolving anti-aliasing, for the first). It doesn't see depth or the original frame (on OpenGL the frame has no alpha either). It is drawn with the previous image on texture tile 0 and a plain `TEXEL0` combiner, so its context has `o_textures[0]` and nothing else of note.

| Input | OpenGL | Vulkan | DirectX 11 | Metal |
|---|---|---|---|---|
| Previous image | `uniform sampler2D uTex0;` | `layout(set = 1, binding = 0) uniform sampler2D uTex0;` | `g_texture0`, `g_sampler0` | `uTex0`, `uTex0Smplr` |
| Coordinate | `vTexCoord0` | `layout(location = 0) in vec2 vTexCoord0;` | `uv0 : TEXCOORD0` | `in.texCoord0` |

Players turn passes on and off in **Shaders › Shader Packs › Popout Shader Settings**, until Ghostship closes; each start uses `enabled`. The built-in `srgb.shader.*` files are complete passes for all four backends and the best starting point.

### Material shaders

A material shader draws its display list, and the ones it calls, instead of `default.shader`. It replaces the template completely and gets the same context, so start from a copy of `default.shader` for each backend and change what you need. The `dlist` is matched when a display list in the archives calls it; display lists the game draws directly from code aren't matched yet.

### Settings

```glsl
@setting(var=STRENGTH, name='Vignette strength', type=float, default=0.5, min=0.0, max=1.0)
```

| Key | Meaning |
|---|---|
| var | Required. The variable the shader reads. |
| name | Label in Shader Settings, under the pack's name. Defaults to `var`. |
| type | `float` (default) or `int`, as sliders; `toggle`, a checkbox; `enum`, a list; `color`, a color picker. |
| default, min, max | Starting value and slider range. A `color` default is `'r, g, b'` from 0 to 1. |
| options | For `enum`: `'Low:0\|Medium:1\|High:2'`. |

Values in single quotes may contain commas. The value is written into the source before it compiles, not passed as a uniform: `@{STRENGTH}` where you need it, `@if(QUALITY == 2)` to pick code. A `toggle` is 0 or 1, an `enum` its number, a `float` a float literal and a `color` the text `r, g, b`, so write `vec3(@{TINT})`. Sliders apply when released, the others at once; the value is saved as `gShaderSettings.<pack>.<var>` and every shader recompiles.

### Example: a vignette pass

`assets/shaders/vignette/vignette.glsl`, for OpenGL and Vulkan; for DirectX and Metal, start from `shaders/directx/srgb.shader.hlsl` and `shaders/metal/srgb.shader.metal`.

```glsl
@prism(type='fragment', name='Vignette', version='1.0.0', description='Darkens the edges', author='You')
@setting(var=STRENGTH, name='Vignette strength', type=float, default=0.5, min=0.0, max=1.0)
@if(BACKEND_VULKAN)
#version 450
@include("shaders/vulkan/include/common.glsli")
@if(VERTEX_SHADER)
    @include("shaders/vulkan/include/fast3d_vs.glsli")
@else
    layout(location = 0) out vec4 vOutColor;
    layout(location = 0) in vec2 vTexCoord0;
    layout(set = 1, binding = 0) uniform sampler2D uTex0;

    void main() {
        vec4 c = texture(uTex0, vTexCoord0);
        vec2 d = vTexCoord0 - 0.5;
        vOutColor = vec4(c.rgb * (1.0 - @{STRENGTH} * dot(d, d) * 2.0), 1.0);
    }
@end
@else
@{GLSL_VERSION}
@if(VERTEX_SHADER)
    @include("shaders/opengl/include/fast3d_vs.glsli")
@else
    @if(core_opengl || opengles)
    out vec4 vOutColor;
    @end
    @{attr} vec2 vTexCoord0;
    uniform sampler2D uTex0;

    void main() {
        vec4 c = @{texture}(uTex0, vTexCoord0);
        vec2 d = vTexCoord0 - 0.5;
        @{vOutColor} = vec4(c.rgb * (1.0 - @{STRENGTH} * dot(d, d) * 2.0), 1.0);
    }
@end
@end
```

### When something goes wrong

Shader errors aren't recoverable: every one of these stops the game, so test a pack on each backend it ships for.

| Symptom | Cause |
|---|---|
| "Failed to load … shader … missing f3d.o2r?" (or "missing shaders/vulkan in the o2r?") | No file for the active backend at the shader's path, or a wrong path in the manifest. |
| "Invalid prism file", "Type not specified" | The file, or a file it includes, doesn't start with `@prism(type=…)`. |
| "Unknown variable X", "Unsupported function call" | A name the context doesn't have for this backend or stage, used outside a branch that excludes it. |
| "Failed to load include from …" | An `@include` path that doesn't exist, even in an untaken branch. |
| "Vulkan shader compilation failed" | A GLSL error; the generated source is printed with line numbers. |
| "Fragment shader compilation failed", "Shader program link failed" | A GLSL error on OpenGL. On GLES, check `uCustom` is declared `highp` in both stages. |
| A message box with HLSL errors | An HLSL error on DirectX. |
| Garbled geometry | A custom vertex stage that doesn't call `update_floats` like the default, so the vertex stride is wrong. |
| Code after an `@if` block disappears | An `@elseif` branch was taken; move the chain to the end of its block. |

---

## Romhacks

Ghostship plays supported Super Mario 64 romhacks. A romhack is the **base game**: the player picks Super Mario 64 or one installed romhack in **Mods**, and mods load on top of it. Only one is active at a time, and switching needs a restart. Everything the hack changed comes from the player's own ROM and is extracted once; nothing from a hack ships with Ghostship.

### Playing a romhack

Ghostship has to be set up with the base game first: the first start asks for a Super Mario 64 ROM and builds `sm64.o2r`, and a romhack loads on top of it. Picking a romhack ROM at that point is refused with *Base Game Required*.

The **Base Game** list in **Mods** shows Super Mario 64, every installed romhack, and every hack this build supports that isn't installed yet. Each row has a link to the hack's download page; only the listed version of a hack is supported.

| To | Do |
|---|---|
| Install a hack that has **Generate** | Click **Generate** on its row and pick your Super Mario 64 ROM. Ghostship extends it if the hack needs that, applies the hack's patch, checks the result is the supported version and extracts the romhack. It's done once; then **Romhack Ready** offers to play it. |
| Install a hack without **Generate** | Patch it yourself: apply the hack's patch (`.bps`, `.ips`, `.ppf` or `.xdelta`) to your Super Mario 64 ROM with a patcher such as Rom Patcher JS, after extending the ROM with sm64extend or M64 ROM Extender (default settings) if the hack says so. Then click **Pick ROM...** under the list and pick the patched `.z64`. |
| Play a romhack, or the base game again | Click **Edit**, pick it under **Base Game**, click **Apply** and restart Ghostship. The same Edit and Apply turn mods on, off and reorder them. |
| Play one for a single launch | Start Ghostship with `-hack <name>`, the romhack's file name without `.o2r`. The saved choice doesn't change. |
| Remove one | Delete its `.o2r` from `mods/~romhacks/`. Its saves stay, and come back if you add it again. |

While a romhack is active, the menu header reads **Romhack active: NAME** and its row says **Playing**. Each romhack keeps its own saves, apart from the base game's and each other's. A hack this build doesn't know can be added as a **recipe package**, a small `.o2r` with the instructions for extracting it and no game data: put it in `romhacks/` in the Ghostship folder and it joins the list.

| Message | Meaning | What to do |
|---|---|---|
| **Wrong ROM** | **Generate** got a ROM that isn't the one the hack's patch was made for. | Pick the original, unmodified Super Mario 64 ROM (usually US), or the hack's patched `.z64`. |
| **Not a Supported Romhack** | No recipe matches the ROM's SHA1. | Check the hack is listed, that you patched the right base ROM, and that you picked the patched file. Different versions of a hack give different ROMs. |
| **Base Game Required** | A romhack ROM was picked as the base game. | Set Ghostship up with Super Mario 64 first, then add the romhack in Mods. |
| **Romhack Extraction Failed** | Generating the romhack's files failed. | Try again; if it keeps failing, report the message shown. |

| What | Where, inside the Ghostship folder |
|---|---|
| Installed romhacks | `mods/~romhacks/<name>.o2r` |
| Recipe packages | `romhacks/<name>.o2r`, unpacked into `cache/romhacks/<name>/` while extracting |
| Romhack saves | `saves/~romhacks/<name>/` |

Folders starting with `~` are managed by Ghostship.

### Mods on a romhack

Mods load on top of the romhack, so a mod that changes something the hack also changed wins. A mod runs on any base unless its manifest says which ones it works with:

```json
"works_with": ["vanilla"]                                    // Super Mario 64 only
"works_with": ["romhack"]                                    // any romhack, not Super Mario 64
"works_with": [{ "name": "Mario74EE", "version": ">=1.4" }]   // one hack, by name or id
"works_with": ["vanilla", "StarRoad"]                        // either
"works_with": ["*"]                                          // anything, as without the key
```

A version range is a list of comparisons (`>=`, `>`, `<=`, `<`, `=`) separated by spaces or commas; a bare version is a minimum. A mod that doesn't fit the active base isn't loaded. It stays enabled, so it loads again once its base is picked, and the Mods list shows an orange warning with the reason. A romhack dropped into plain `mods/` isn't loaded as a mod.

From C, `port/romhack/Romhack.h` says which base is running:

| Function | Returns |
|---|---|
| bool Romhack_IsActive(void) | Whether a romhack is the base. |
| const char* Romhack_GetId(void) | Its id, e.g. `"StarRoad"`, or NULL on Super Mario 64. |
| const char* Romhack_GetName(void) | Its name, or NULL. |
| const char* Romhack_GetVersion(void) | Its version, or NULL when unknown or on Super Mario 64. |

```c
#include "port/romhack/Romhack.h"

MOD_INIT() {
    if (Romhack_IsActive() && strcmp(Romhack_GetId(), "StarRoad") == 0) {
        // Star Road's levels, not the base game's.
    }
}
```

### Locking port features

A romhack or a mod can turn off Ghostship features that would break it, or pin them to a value, with two manifest keys (a romhack's come from its recipe):

```json
"disable": ["cheats.*", "enhancements.select_all_stars"],
"force":   { "graphics.interpolation_fps": 30 }
```

- `disable` takes feature ids or `group.*`. It adds up over every source: the feature reads as its vanilla value, and its menu item is greyed out with a tooltip naming who disabled it.
- `force` pins a value; toggles and numbers take a number or `true`/`false`. If two sources force different values, the later one in load order (built-in rules, the romhack, then the mods by priority) wins, and Mods lists the conflict. A forced value wins over a disable.
- The randomizer (`rando.*`) is always disabled while a romhack is the base. `settings.*` (audio, controls, menus, the renderer, the romhack and mod choice) can't be locked, and unknown ids are logged and ignored.
- Nothing is written to the player's settings: their own value comes back once the romhack or mod is gone.

| Id | Setting | Vanilla value |
|---|---|---|
| cheats.infinite_health, cheats.infinite_lives | Infinite health and lives | 0 |
| cheats.always_fly_triple_jump, cheats.flying_triple_jump_high_launch | Triple jump flying | 0 |
| cheats.pause_exit_whenever, cheats.play_in_demo | Exit course anytime, play in the demo | 0 |
| enhancements.disable_draw_distance, enhancements.disable_lod | Draw distance and level of detail | 0 |
| enhancements.disable_lakitu_cutscene, enhancements.disable_peach_cutscene | Skipped cutscenes | 0 |
| enhancements.select_all_stars, enhancements.star_no_exit | Pick any star, stay in the level after a star | 0 |
| enhancements.always_show_power_meter | Power meter always shown | 0 |
| enhancements.mirrored_world | Mirror mode | 0 |
| enhancements.fix_koopa_race_music, fix_heave_ho_sinking, fix_yoshi_dialogue_typo, fix_file_select_button_tilt | Bug fixes (each under `enhancements.`) | 0 |
| camera.default_mode, camera.alternate_mode, camera.improved_c_buttons, camera.horizontal_analog, camera.vertical_analog, camera.center_button | Camera options | 0 |
| graphics.alt_assets | Alternate assets | 0 |
| graphics.interpolation_fps, graphics.match_refresh_rate | Frame rate | 30, 0 |
| rando.enabled | Randomizer | 0 |
| devtools.debug_level_select, devtools.better_level_select, devtools.save_editor, devtools.object_viewer | Developer tools and windows | 0, closed |

From C, read a lockable setting through the registry rather than its console variable: `Port_FeatureInt(id, fallback)` and `Port_FeatureFloat(id, fallback)` return the forced value, else the vanilla value while disabled, else the setting; `Port_FeatureAllowed(id)` is false while anything disables it, and `Port_FeatureDisabledBy(id)` names who did.

### Game data events

Rules the game has compiled in (star requirements, boss star spots, paths, camera tuning) are read through events a mod can answer; a romhack's own code uses them to apply what the hack changed. Each passes a pointer to the vanilla value: write it to change it.

| Event | Changes |
|---|---|
| GameValue, GameFloat | Numbers and switches by `GameValueId` / `GameFloatId`: star counts for doors, Toads and MIPS, starting lives, King Bob-omb's health, where Exit Course goes, where a new file starts, whether the intro plays, camera behaviour… |
| SpawnFixedStar | Where a boss or minigame star appears (`FixedStarId`), or cancel it to spawn something else, such as a key. |
| ModifyTrajectory | The paths Koopa the Quick, bowling balls, racing penguins and track platforms follow. |
| CameraCourseProcessing | Cancel to turn the level's camera triggers off. |
| TitleScreenFrame | Each title screen frame; cancel to hide Mario's head. |
| CreditsEntryLines | The text of each credits entry. |

```c
static void on_game_value(IEvent* event) {
    GameValue* ev = (GameValue*) event;
    if (ev->id == GAME_VALUE_STARTING_LIVES) {
        *ev->value = 3;
    }
}

MOD_INIT() {
    REGISTER_LISTENER(GameValue, EVENT_PRIORITY_NORMAL, on_game_value);
}
```

### Level and behavior scripts

Level scripts and behavior scripts can be loaded from an archive instead of being compiled in. A romhack replaces levels and adds behaviors this way, and any mod can do the same by shipping the files.

| Path in the archive | Used for |
|---|---|
| scripts/levels/<folder> | Replaces the level whose script is `level_<folder>_entry` (folders as in `levels/level_defines.h`: `bob`, `castle_inside`…). Whenever the game is about to run a level, it checks the archives for this first; the last archive that ships it wins. |
| scripts/behaviors/<name> | A behavior other scripts call `<name>`, loaded the first time one does. |
| textures/skyboxes/levels/<folder>/ptrlist | The level's own skybox, 80 tile pointers, used over the one its geo layout names. |
| levels/<folder>/<folder>_water_<id> | ROM Manager water boxes, geo layout ids `0x5000` to `0x5003`. |

A script is a list of arrays of commands named after the decomp's macros (`OBJECT_WITH_ACTS`, `WARP_NODE`, `BEGIN`, `CALL_NATIVE`…). Each command expands through the same macro the compiler uses, so a loaded script is identical to a compiled one. Arguments are:

| Argument | Example | Becomes |
|---|---|---|
| A number or constant expression | `0x00010000`, `OBJ_FLAG_HOLDABLE \| OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE`, `MODEL_STAR` | its value |
| A name | `bhvStar`, `bhv_star_door_loop`, `script_func_global_1`, another array in the script | a pointer to it |
| A string | `"levels/bob/geo/bob_geo_000488"` | that asset from the archives |

Known names are every vanilla behavior, every function a vanilla behavior or level script calls, the global level scripts, level and course numbers, and the constants from `model_ids.h`, `object_constants.h`, `object_fields.h` (fields as indices), `seq_ids.h`, `dialog_ids.h`, `surface_terrains.h`, `level_commands.h`, `geo_commands.h`, `interaction.h`, `object_list_processor.h` and `shadow.h`. Any other name is looked up as `scripts/behaviors/<name>`. A code mod adds its own with `Scripts_RegisterPointer(name, pointer)` and `Scripts_RegisterConstant(name, value)` from `port/scripts/Scripts.h`, so its natives can be called from scripts:

```c
static void bhv_my_platform_loop(void) { /* ... */ }

MOD_INIT() {
    // Behavior scripts can now CALL_NATIVE(bhv_my_platform_loop).
    Scripts_RegisterPointer("bhv_my_platform_loop", (const void*) bhv_my_platform_loop);
}
```

Scripts you write are XML files of decomp source. The root is `LevelScript` or `BehaviorScript`; `Entry` names the array to start from (by default `*_entry` for levels and the first array for behaviors). Comments, preprocessor lines and `extern` declarations are skipped, so a decomp file pastes in once its assets are written as strings; resolve `#ifdef` blocks first. In the archive the file goes without its extension: `scripts/behaviors/bhvSpinningCoin`.

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

Scripts extracted from a ROM are in a binary form Torch writes: after the resource header, the array count and the entry name, then each array's name and commands, each command's name, source line and arguments, each argument a type byte (0 number, 1 name, 2 asset path) and an `s64` or a string.

### How it works

```
  Super Mario 64 ROM                 Patched romhack ROM, or the base ROM + a recipe package
          |                                         |
     first start                       Mods: Generate, or Pick ROM...
          |                                         |  (refused if no recipe has its SHA1)
          v                                         v
  sm64.o2r: the whole game        mods/~romhacks/<name>.o2r: what the hack changed,
          |                       its scripts, its code's RAM image, its code mod
          +--------------------+--------------------+
                               v
     boot: mount sm64.o2r, the romhack, then the mods; build the code mods
```

- **Load order**: the base game, then the romhack, then the mods by priority. Files are looked up by path, so the romhack shadows the base game and mods shadow both. The selected romhack is the setting `gSettings.Romhack.Selected`; `-hack` overrides it for one launch.
- **Identity**: a romhack's archive has a `manifest.json` with its `name`, `version`, `author`, `description`, `website`, its `disable` / `force` rules, and a `romhack` block with its `id` and the ROM's `rom_sha1`. The game never needs the recipe at runtime. With a code mod, the manifest also has its `main`, `code_version` and `api_version`, so the hack's code builds from its own archive like any code mod. The Mods list leaves the active romhack out.
- **Saves**: `saves/~romhacks/<name>/`. The old `default.sav` migration only runs for the base game.

A romhack reaches the game through four layers:

| Layer | How | Example |
|---|---|---|
| Assets: textures, models, geo layouts, collision, text, music | YAMLs for what the hack changed, extracted into its archive | new textures and level geometry |
| Game data compiled into Ghostship | game data events, answered by the hack's code mod from the RAM image of its code | star requirements, boss stars, paths, camera tuning |
| Scripts: levels, behaviors built from vanilla code | extracted from the ROM, or script files the recipe ships | its levels, objects and warps |
| Custom code: new natives, changed rules | a C mod in the hack's archive, through events | new object logic, hard modes |

Nothing hack-specific is compiled into Ghostship. A hack without code still gets its assets and scripts.

### Porting a romhack

A supported romhack is a folder, `tools/romhacks/<name>/`, holding everything about it but its data:

| File | What it is |
|---|---|
| recipe.yml | The extraction recipe, below. |
| yamls/ | Torch YAMLs for what the hack changed, including the RAM image of its code (`romhack/ram_<address>`). |
| symbols/<name>.yml | Names for the functions the hack added in assembly, so its extracted scripts can call them. |
| src/, include/, manifest.json | Its code: a C mod laid out like the mod template. |
| README.md | How to regenerate it. |

```yaml
<SHA1 of the patched ROM>:
  name: Super Mario Star Road            # shown in menus, and names its saves
  path: yamls
  symbols: [ assets/symbols/us.yml, symbols/star_road.yml ]
  romhack:
    id: StarRoad
    version: "Final"
    author: Skelux
    description: One line about the hack.
    website: https://…                   # the download page, linked from Mods
    disable: [ "cheats.*" ]              # optional
    force: { graphics.interpolation_fps: 30 }   # optional
    files: scripts                       # optional: a folder packed into the archive as is
    base:                                # optional: lets Mods offer Generate
      sha1: <SHA1 of the ROM the patch was made for>
      extend: true                       # rebuild the sm64extend base first
      patch: Super Mario Star Road.ppf
  config:
    gbi: F3D
    sort: OFFSET
    gbi_floats: true
    output:
      binary: mods/~romhacks/star_road.o2r
      code: code/romhacks/star_road
      headers: include/assets
```

The recipe is keyed by the SHA1 of the patched ROM, which is how Ghostship recognizes it. Paths are relative to the folder, except shared files like `assets/symbols/us.yml`. A recipe may also have a `segments:` table, the hack's own.

`rom_manager.py package <folder> <out.o2r>` packs the folder into a recipe package: the recipe as `config.yml`, the YAMLs, the symbols and the built code mod. With `base`, and the patch file next to the recipe or at the repo root, the package also carries the patch and an entry for the base ROM whose Torch preprocess jobs rebuild the hack (`sm64extend`, then `patch`: PPF3, IPS, BPS or xdelta), check the result's SHA1 and extract it; that's what **Generate** does. The build packs every `tools/romhacks/*/recipe.yml` into `romhacks/` next to the game. From the command line, `Ghostship --extract-romhack <rom> [--recipe <name>]` extracts without the menu. A patch is its author's work: share a package that carries one only where the author allows it. Patch files are ignored by git.

The hack's code mod builds like any other and ships inside the romhack's archive. It runs once per launch and, since switching romhacks needs a restart, never has to unregister its listeners:

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
    Scripts_RegisterPointer("bhv_my_hack_platform_loop", (const void*) bhv_my_hack_platform_loop);
    REGISTER_LISTENER(GameValue, EVENT_PRIORITY_NORMAL, on_game_value);
}

MOD_EXIT() {
}
```

### ROM Manager hacks

Most Super Mario 64 romhacks were made with ROM Manager or SM64 Editor, which extend the ROM to 64 MB and move level data into the new space, so a plain diff against the base ROM makes the whole game look rewritten. `tools/romhacks/rom_manager.py` understands ROM Manager's layout:

- **Extended base**: the hack is built on an sm64extend ROM, and its patch usually only stores the bytes that differ from it, so applying it to a plain 8 MB ROM gives broken bytes all over. `extend` rebuilds that base exactly (sm64extend and M64 ROM Extender 1.3b, default settings). Some hacks, such as Super Mario 74's xdelta, patch the plain ROM and produce the extended one themselves.
- **Levels**: each level the hack replaced has its own block, script in segment 0x19, area geometry in 0x0E, texture bank in 0x09 (MIO0 blocks stored uncompressed). Levels it edited where vanilla keeps them, with vanilla data, are found by comparing with the base ROM.
- **Skyboxes**: each level loads its own into segment 0x0A, and hacks reuse or add background ids, so each level gets `textures/skyboxes/levels/<folder>/ptrlist`.
- **Title logo**: the intro keeps its vanilla script, and the hack's logo comes in under the vanilla resource names.
- **Custom code**: ROM Manager's own code and the hack's patches to the game's code are in its RAM image, `romhack/ram_<address>`; the code mod reads the hack's numbers from there instead of repeating them, checking each patch is the one it expects. Assembly the hack added is ported by hand, and named in its symbols file.

```sh
# The hack's ROM: the base it was made against, then the patch (PPF3 or xdelta)
python3 tools/romhacks/rom_manager.py extend baserom.us.z64 sm64.ext.z64
python3 tools/romhacks/rom_manager.py patch sm64.ext.z64 "My Hack.ppf" my_hack.z64

# The YAMLs, with the level and behavior scripts and the RAM image
python3 tools/romhacks/rom_manager.py yamls my_hack.z64 tools/romhacks/my_hack/yamls --yaml-root yamls --scripts \
    --base-rom baserom.us.z64 --symbols assets/symbols/us.yml --symbols tools/romhacks/my_hack/symbols/my_hack.yml

# The recipe package, and the romhack itself without the menu
python3 tools/romhacks/rom_manager.py package tools/romhacks/my_hack romhacks/my_hack.o2r
Ghostship --extract-romhack my_hack.z64
```

`yamls` follows the level scripts and writes one YAML per level (area geo layouts, collision, macro objects, lights, textures), one per texture bank, the music, text and Mario if the hack changed them, and with `--scripts` each level's script, the models it loads and every behavior the hack rewrote in place. Behaviors the hack left alone stay vanilla. It prints what it left out and why; `--extra` adds assets the scripts don't point at, such as collision a custom behavior loads. `tools/romhacks/star_road` and `tools/romhacks/mario74` are complete examples.

Other hacks differ: SM64 Editor and Quad64 hacks use their own layouts, and decomp-based hacks ship a patch to the decomp source, whose assets build like the base game's and whose code changes become the hack's code mod.

---

## Ghostship APIs

The following C APIs are provided by Ghostship (not libultraship) and are available to mods running inside the Ghostship port. Include the relevant header from `src/port/api/` and declare it `extern` in your mod, or use the amalgamation pipeline to inline it.

All functions use C linkage and are safe to call from C99/C11 source files.

---

### UI API

**Header:** `port/api/ui.h`

Lets mods add entries and widgets to the Ghostship settings menu.

#### Types

```c
typedef enum {
    C_WIDGET_CHECKBOX,
    C_WIDGET_COMBOBOX,
    C_WIDGET_SLIDER_INT,
    C_WIDGET_SLIDER_FLOAT,
    C_WIDGET_CVAR_CHECKBOX,
    C_WIDGET_CVAR_COMBOBOX,
    C_WIDGET_CVAR_SLIDER_INT,
    C_WIDGET_CVAR_SLIDER_FLOAT,
    C_WIDGET_BUTTON,
    C_WIDGET_INPUT,
    C_WIDGET_CVAR_INPUT,
    C_WIDGET_CVAR_COLOR_PICKER,
    C_WIDGET_COLOR_PICKER,
    C_WIDGET_SEPARATOR,
    C_WIDGET_SEPARATOR_TEXT,
    C_WIDGET_TEXT,
    // ...
} C_WidgetType;
```

`C_WidgetConfig` is a tagged union struct. Set `type` to select which `opts` member is active:

```c
typedef struct {
    C_WidgetType  type;
    const char*   cvar;         // for CVAR_* types
    const char*   window_name;  // for WINDOW_BUTTON type
    bool          race_disable;
    bool          same_line;
    bool          hide_in_search;
    bool          is_hidden;
    void (*callback)(void);
    void (*pre_func)(void);
    void (*post_func)(void);
    union {
        C_ComboboxOpts    combo;
        C_SliderIntOpts   slider_int;
        C_SliderFloatOpts slider_float;
        C_CheckboxOpts    checkbox;
        C_TextOpts        text;
        C_ColorPickerOpts color_picker;
        C_ButtonOpts      generic;
    } opts;
} C_WidgetConfig;
```

#### Functions

| Function | Description |
|---|---|
| `void C_AddSidebarEntry(const char* label, int column)` | Adds a sidebar section under **Mods**. `column` is `1` or `2`. |
| `void C_AddWidget(const char* sidebar, int column, const char* label, C_WidgetConfig* config)` | Adds a widget under a sidebar entry. |
| `void C_RemoveSidebarEntry(const char* sidebarName)` | Removes a sidebar entry and all its widgets. |

#### Example

```c
#include "mod.h"
#include "port/api/ui.h"

static void on_toggle(void) { /* respond to checkbox change */ }

MOD_INIT() {
    C_WidgetConfig chk = {0};
    chk.type = C_WIDGET_CVAR_CHECKBOX;
    chk.cvar = "gMyMod.Enabled";
    chk.opts.checkbox.tooltip = "Enable my mod feature.";
    chk.opts.checkbox.default_val = true;
    chk.callback = on_toggle;

    C_AddSidebarEntry("My Mod", 1);
    C_AddWidget("My Mod", 1, "Enable Feature", &chk);
}

MOD_EXIT() {
    C_RemoveSidebarEntry("My Mod");
}
```

---

### Achievements API

**Header:** `port/api/achievements.h`

Lets mods define and track custom achievements that appear alongside the built-in ones in the Achievements window.

#### Types

```c
typedef enum {
    C_ACH_CATEGORY_NONE   = 0,
    C_ACH_CATEGORY_STARS  = 1 << 0,
    C_ACH_CATEGORY_CAPS   = 1 << 1,
    C_ACH_CATEGORY_LEVELS = 1 << 2,
    C_ACH_CATEGORY_BOSSES = 1 << 3,
    C_ACH_CATEGORY_DEATHS = 1 << 4,
    C_ACH_CATEGORY_EXTRAS = 1 << 5,
} C_AchievementCategory;

typedef struct {
    C_AchievementCategory  category;
    const char*            name;
    const char*            icon;         // texture path, e.g. "extras.my-icon"
    const char*            description;
    int32_t                maxProgress;  // goal value; use 1 for binary done/not-done
    const char**           dependencies; // NULL-terminated array of achievement IDs, or NULL
} C_AchievementDef;
```

#### Functions

| Function | Description |
|---|---|
| `void C_RegisterAchievement(const char* id, const C_AchievementDef* def)` | Registers a new achievement. Duplicate IDs are silently ignored. Safe to call at any time after engine init. |
| `void C_AchievementProgress(const char* id, int32_t amount)` | Increments progress on `id` by `amount`. Triggers the unlock notification when progress reaches `maxProgress`. |
| `int32_t C_AchievementGetProgress(const char* id)` | Returns current progress. Returns `0` for unknown IDs. |
| `int C_AchievementIsUnlocked(const char* id)` | Returns `1` if unlocked, `0` otherwise. |

#### Example

```c
#include "mod.h"
#include "port/api/achievements.h"
#include "port/events/Events.h"

static ListenerID gJumpListenerID;
static int32_t    gJumpCount = 0;

static void on_jump(IEvent* event) {
    (void)event;
    gJumpCount++;
    if (gJumpCount % 10 == 0) {
        C_AchievementProgress("MyMod.BigJumper", 10);
    }
}

MOD_INIT() {
    static const C_AchievementDef def = {
        .category    = C_ACH_CATEGORY_EXTRAS,
        .name        = "Big Jumper",
        .icon        = "extras.jumper",
        .description = "Jump 500 times.",
        .maxProgress = 500,
        .dependencies = NULL,
    };
    C_RegisterAchievement("MyMod.BigJumper", &def);

    gJumpListenerID = REGISTER_LISTENER(PlayerSetAction, EVENT_PRIORITY_NORMAL, on_jump);
}

MOD_EXIT() {
    UNREGISTER_LISTENER(PlayerSetAction, gJumpListenerID);
}
```

> **Note:** Achievement progress is automatically saved alongside game saves. Always use a namespaced ID (e.g. `"MyMod.SomeAchievement"`) to avoid collisions with built-in achievements.

---

### Permissions API

**Header:** `port/api/permissions.h`

Lets mods request explicit user consent before performing sensitive operations. Permission decisions are persisted across sessions in `permissions.json` and are never writable through the CVar system.

#### Types

```c
typedef enum {
    PERM_PENDING = 0,  // user has not been asked yet
    PERM_ALLOWED = 1,  // user granted access
    PERM_DENIED  = 2,  // user denied access
} C_PermissionState;

typedef void (*C_PermissionCallback)(void);
```

#### Functions

| Function | Description |
|---|---|
| `void C_RegisterPermission(const char* key, const char* title, const char* message)` | Registers a permission key with the dialog text shown to the user. Idempotent — safe to call multiple times with the same key. |
| `C_PermissionState C_GetPermission(const char* key)` | Returns the current persisted state for `key`. |
| `void C_RequestPermission(const char* key, C_PermissionCallback on_allow, C_PermissionCallback on_deny)` | Shows the permission dialog if state is still `PERM_PENDING`. Either callback may be `NULL`. Does nothing if already decided. |

#### Permission flow

```
First run:
  C_RegisterPermission  →  C_RequestPermission  →  [modal shown]
                                                       ├─ Allow → on_allow(), state saved as PERM_ALLOWED
                                                       └─ Deny  → on_deny(),  state saved as PERM_DENIED

Subsequent runs:
  C_GetPermission → PERM_ALLOWED or PERM_DENIED (no modal shown again)
```

#### Example

```c
#include "mod.h"
#include "port/api/permissions.h"

static void on_allowed(void) {
    /* start the feature that requires permission */
}

MOD_INIT() {
    C_RegisterPermission(
        "mymods.telemetry",
        "My Mod — Telemetry Permission",
        "My Mod wants to send anonymous play-time data to the mod author.\n"
        "No personal information is collected. Allow?"
    );

    if (C_GetPermission("mymods.telemetry") == PERM_ALLOWED) {
        on_allowed();
    } else {
        C_RequestPermission("mymods.telemetry", on_allowed, NULL);
    }
}
```

> **Security note:** The write path for permission decisions is internal to the engine and is only triggered by the user clicking a button in the modal. It is not reachable through `CVarSetInteger` or any other scripting API.

---

### Relay API

**Header:** `port/api/relay.h`

Lets mods send and receive binary messages over the Satella WebSocket relay. Each channel is isolated — messages sent on one channel are only delivered to other subscribers of that same channel.

Before any data can flow, the **user must grant permission** through a modal prompt (backed by the Permissions system). The decision is persisted; the prompt only appears once per channel per installation.

#### Types

```c
typedef enum {
    RELAY_PERM_PENDING = 0,
    RELAY_PERM_ALLOWED = 1,
    RELAY_PERM_DENIED  = 2,
} C_RelayPermission;

// Called when a message arrives on the channel.
// `data` is the raw payload; `size` is its byte length.
typedef void (*C_RelayCallback)(const char* channelId, const char* data, uint32_t size);

typedef struct {
    const char*     mod_name;    // shown in the permission dialog
    const char*     description; // one-sentence explanation shown to the user
    C_RelayCallback on_message;
} C_RelayChannelDef;
```

#### Functions

| Function | Description |
|---|---|
| `void C_RegisterRelayChannel(const char* channelId, const C_RelayChannelDef* def)` | Registers a channel. Duplicate IDs are ignored. Safe to call from `MOD_INIT`. |
| `int C_RelaySend(const char* channelId, const char* data, uint32_t size)` | Sends `size` bytes to all other subscribers of the channel. Returns `1` on success, `0` if not permitted, not connected, or validation failed. |
| `C_RelayPermission C_RelayGetPermission(const char* channelId)` | Returns the current permission state for the channel. |
| `void C_RelayRequestPermission(const char* channelId)` | Shows the permission modal if state is still `RELAY_PERM_PENDING`. No-op if already decided. |

#### Channel ID rules

Channel IDs must be non-empty strings of at most 128 characters using only `[a-zA-Z0-9._-]`. Use reverse-domain notation to avoid collisions: `"mymods.positions"`, `"mymods.chat"`.

#### Send limits

`C_RelaySend` rejects payloads larger than **64 KB**. Payloads of size `0` are also rejected.

#### Example

```c
#include "mod.h"
#include "port/api/relay.h"

typedef struct { float x, y, z; } Vec3;

static void on_relay_message(const char* channel, const char* data, uint32_t size) {
    (void)channel;
    if (size < sizeof(Vec3)) {
        return;
    }
    Vec3 pos;
    memcpy(&pos, data, sizeof(Vec3));
    /* render ghost at pos.x, pos.y, pos.z */
}

MOD_INIT() {
    static const C_RelayChannelDef def = {
        .mod_name    = "Ghost Positions Mod",
        .description = "Syncs player positions with other connected players.",
        .on_message  = on_relay_message,
    };
    C_RegisterRelayChannel("mymods.positions", &def);
    /* Permission prompt is shown automatically on first run. */
}

/* Call this from your game-loop listener to broadcast the local player position. */
void SendPosition(float x, float y, float z) {
    Vec3 pos = { x, y, z };
    C_RelaySend("mymods.positions", (const char*)&pos, sizeof(pos));
}
```

> **Note:** The engine handles connection, reconnection, and subscriber cleanup automatically. Mods only need to register their channel and call `C_RelaySend` when they have data to send.
