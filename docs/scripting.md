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
10. [Ghostship APIs](#ghostship-apis)
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
