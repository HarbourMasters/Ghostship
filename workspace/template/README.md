# **Harbour Masters C Mod Template**

A streamlined template for creating C-based mods for games utilizing the libultraship engine (e.g., Super Mario 64 PC ports).

This template utilizes a Unity Build (amalgamation) pipeline. It allows you to write clean, modular C code organized across multiple directories, and automatically compiles them down into a single, portable mod.c file. This single-file output is perfectly formatted for dynamic compilation and loading via TCC (Tiny C Compiler) or for easy distribution.

## **🌟 Features**

* **Automated Amalgamation:** Uses a custom Python script hooked into CMake to safely merge all your .c and .h files, preventing redefinition errors.  
* **Cross-Platform Exports:** Includes HM_API macros to handle the exact ABI export rules so the engine's dynamic linker can always find your ModInit function.  
* **Event Hooking:** Includes a working boilerplate for tapping into the engine's event system (e.g., GameFrameUpdate, RenderGamePost).

## **📂 Directory Structure**

```
├── CMakeLists.txt        # The build configuration  
├── include/  
│   └── mod.h              # Core macros (MOD_INIT, MOD_EXIT, HM_API)  
├── src/  
│   └── demo.c             # Your actual mod logic and event listeners  
└── output/  
    ├── dist/             # The final c files goes here after building
    ├── build.gen         # The list of how to build the mod (auto-generated)
    └── manifest.json      # Metadata for the mod loader
```

## **🚀 Getting Started**

### **Prerequisites**

* **CMake** (3.10 or higher)  
* **Python 3** (used for the amalgamation step)  
* The libultraship source headers available in your parent directory structure.

### **Building the Mod**

We use an out-of-source build to keep the working directory clean. The CMake script will automatically grab all files in the src/ directory and generate the single mod.c in the output/ folder.

1. Open your terminal in the project root.  
2. Create a build directory and configure the project:  
   ```Bash  
   mkdir build  
   cd build  
   cmake ..
   ```

3. Run the build process:  
   ```Bash
   cmake --build .
   ```

4. Your fully amalgamated mod will be available at output/mod.c, ready to be packaged alongside the generated output/manifest.json.

## **🛠️ Writing Your Mod**

### **The Entry and Exit Points**

Every mod requires an entry point so the engine knows how to initialize it. Use the provided macros from mod.h:

```C
#include "mod.h"

MOD_INIT() {  
    // Called when the mod is dynamically loaded.  
    // Register your engine listeners here.  
}

MOD_EXIT() {  
    // Called when the mod is unloaded or the game closes.  
    // Undo anything MOD_INIT did outside the port/api headers.  
}
```

When a mod unloads, everything it registered through the `port/api` headers is unregistered after its `MOD_EXIT`, along with its event listeners, menu entries and GUI draw callbacks. `MOD_EXIT` is optional and only needs to undo what the mod set up by other means. Mods are unloaded and rebuilt when "Reload Scripts" is pressed in the Mods menu.

### **Hooking into Engine Events**

The template includes a demo.c file showing how to hook into the engine's game loop.

* **GameFrameUpdate**: Ideal for gameplay logic (e.g., modifying gMarioState).  
* **RenderGamePost**: Ideal for drawing custom UI (e.g., using print_text_centered).

`UNREGISTER_LISTENER` removes a listener while the mod runs; the ones still registered when it unloads are removed for it.

### **Settings and Menus**

Use `port/api/cvar.h` for settings and keep them under the mod's own prefix, `gMods.<mod>.`; `C_MOD_CVAR("mymod", "Speed")` spells it out. Widget tooltips and CVar names are kept as pointers, so pass string literals.

`port/api/ui.h` adds sidebars and widgets to the Mods menu, and with `C_AddGuiDraw` draws windows and overlays of your own with the `C_Gui*` functions. `C_WorldToScreen` gives where a point in the level shows on screen, e.g. for a label over an object.

### **The Mods Menu**

**Mods** lists every mod with a manifest, enabled or not, with its state: loaded, failed to build (with the compiler's error), skipped (with the reason, e.g. a missing dependency) or disabled. Each mod has an **Enabled** switch and a **Priority**. Mods with a higher priority are mounted later, so their files replace those of mods before them, and their code initializes later; dependencies always initialize first. Both take effect when Ghostship restarts. A mod that fails to build doesn't stop the others from loading.

### **Dependencies**

A mod that needs another lists it by its manifest name in `manifest.json`. A version range is optional:

```json
"dependencies": ["Other Mod", { "name": "Core Library", "version": ">=1.2 <2" }]
```

Dependencies initialize first. A range is a list of comparisons (`>=`, `>`, `<=`, `<`, `=`); a version without one is a minimum. A mod whose dependency is missing, failed to build or is outside the range is skipped with a notification, and so are the mods that depend on it. Call into another mod with `CALL_FUNC` from `mod.h`.

### **Versions**

The build writes `code_version` and `api_version` to the packaged `manifest.json` from `src/port/api/api.h`. A mod loads only in a Ghostship with the same code version and at least its API version; rebuild it when the code version changes. The `C_Register*` and `C_AddWidget` macros pass the size of the definition the mod was built with, so fields added to the end of a definition in later versions read as zero for older mods.

### **The Mod SDK**

The template builds inside a Ghostship checkout, in `workspace/<your-mod>`, or inside the mod SDK: a zip with the same layout holding only the headers, the mod tools and this template. `cmake --build build --target mod_sdk` in a Ghostship checkout writes it as `ghostship-mod-sdk-code<code version>-api<API version>.zip`.

### **⚠️ Important Note on Unity Builds (Amalgamation)**

Because all of your .c files are ultimately merged into a single mod.c file, everything shares the same global scope.

* **Use static:** If you write a helper function that should only be used internally, **always** prefix it with static. This ensures it won't accidentally collide with functions of the same name in the game engine or other mods once loaded into memory.