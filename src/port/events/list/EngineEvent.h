#pragma once

#include "engine/graph_node.h"
#include "engine/level_script.h"

typedef s32 (*LevelScriptFunction)(s16, s32);

// ────────────────────────────────────────────────────────────────────────────
// Engine lifecycle
// ────────────────────────────────────────────────────────────────────────────

// Fires once at the end of GameEngine::FinishInit(), after all scripts are loaded.
DEFINE_EVENT(EngineReady);

// Fires each main-loop iteration before game logic runs.
DEFINE_EVENT(GameLoopTick);

// Fires once per game logic frame in level_update_frame().
DEFINE_EVENT(GameFrameUpdate);

// Fires before controller inputs are read and processed. Cancellable.
// Cancel — skips reading all controllers this frame.
DEFINE_EVENT(GameReadInput);

// Fires when a keyboard key is pressed (desktop only).
DEFINE_EVENT(KeyboardInput,
    uint32_t scancode;  // platform scancode of the pressed key
);

// ────────────────────────────────────────────────────────────────────────────
// Render pipeline
// ────────────────────────────────────────────────────────────────────────────

// Fires before the game's main render pass begins.
DEFINE_EVENT(RenderGamePre);

// Fires after the game's main render pass completes.
DEFINE_EVENT(RenderGamePost);

// Fires during HUD rendering. Cancellable.
// Cancel — suppresses the entire HUD for this frame.
DEFINE_EVENT(RenderHud);

// Fires during in-world text-label rendering. Cancellable.
// Cancel — suppresses all text labels for this frame.
DEFINE_EVENT(RenderTextLabels);

// Fires in the pause menu to decide whether "Exit Course / Continue" is shown.
// Write *render = true to force the panel visible regardless of current course.
DEFINE_EVENT(RenderPauseCourseOptions,
    bool* render;  // set to true to force the options panel visible
);

// Fires each frame in the file-select behavior loop.
DEFINE_EVENT(FileSelectRender);

// Fires at the end of update_camera() every frame after Lakitu state is resolved.
DEFINE_EVENT(CameraUpdate,
    struct Camera* c;  // current camera state
);

// ────────────────────────────────────────────────────────────────────────────
// Level script
// ────────────────────────────────────────────────────────────────────────────

// Fires when a level script entry point is about to execute. Cancellable.
// Write *cmd to redirect to a different script. Cancel — skips default entry point.
DEFINE_EVENT(LevelScriptEntry,
    struct LevelCommand** cmd;  // mutable pointer to the entry command
);

// Fires for every level script command dispatched. Cancellable.
// Read (*cmd)->type for the opcode and (*cmd)->size for the command length.
// Write *cmd to redirect or skip commands. Cancel — skips executing this command.
DEFINE_EVENT(LevelScriptExecute,
    struct LevelCommand** cmd;  // mutable pointer to sCurrentCmd
);

// Fires inside the level script loop handler.
// Write *func or *arg to redirect the loop call.
DEFINE_EVENT(LevelScriptCallLoop,
    LevelScriptFunction* func;  // mutable loop function pointer
    int16_t* arg;               // mutable argument passed to the function
);

// Fires when the level script opens a new area.
// Write *geoLayoutAddr to substitute a custom geo layout.
DEFINE_EVENT(LevelScriptBeginArea,
    uint8_t* areaIndex;      // mutable area index
    void** geoLayoutAddr;    // mutable pointer to the geo layout address
);

// Fires when the game initialises a level from the save file.
DEFINE_EVENT(LevelInitFromSaveFile);

// Fires to allow replacing the level script for a given level number.
// Write *newScript to substitute a different script.
DEFINE_EVENT(LevelScriptOverride,
    s16 levelNum;           // level being loaded
    const void** newScript; // mutable script pointer; write to replace
);

// Fires when a save file slot is loaded. fileNum is 0-based.
DEFINE_EVENT(OnGameFileLoad,
    s32 fileNum;
);

// Fires immediately after save data is flushed to disk. fileNum is 0-based.
DEFINE_EVENT(OnGameFileSave,
    s32 fileNum;
);

// ────────────────────────────────────────────────────────────────────────────
// Geo layout — command-level hooks (C and OTR paths)
// ────────────────────────────────────────────────────────────────────────────

// Fires at the start of process_geo_layout() / GeoLayoutParser::execute().
DEFINE_EVENT(GeoLayoutBegin,
    void* segptr;  // segmented pointer or OTR path (cast accordingly)
);

// Fires at the end of process_geo_layout() after the scene graph is built.
DEFINE_EVENT(GeoLayoutEnd,
    struct GraphNode* rootNode;  // root of the built scene graph
);

// Fires when a node_generated or node_background node registers an ASM callback.
// Write *func to redirect the callback. Write *parameter to change the node arg.
// Cancellable — cancel to suppress registering the node entirely.
DEFINE_EVENT(GeoLayoutCallASM,
    GraphNodeFunc* func;      // mutable callback function pointer
    int16_t*       parameter; // mutable node parameter
);

// Fires when a perspective (camera frustum) node is about to be created.
// All fields are mutable. Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodePerspective,
    f32*          fov;         // field of view in degrees
    s16*          near;        // near clipping plane distance
    s16*          far;         // far clipping plane distance
    GraphNodeFunc* frustumFunc; // optional frustum callback (may be NULL)
);

// Fires when a camera scene graph node is about to be created.
// All fields are mutable. Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodeCamera,
    s16*          type;  // camera type (course-specific)
    f32*          pos;   // camera position (Vec3f, 3 floats)
    f32*          focus; // camera focus/target (Vec3f, 3 floats)
    GraphNodeFunc* func; // camera update callback
);

// Fires when a switch-case node is about to be created.
// Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodeSwitchCase,
    s16*          initialCase; // which child variant is active at start
    GraphNodeFunc* func;       // callback that selects the active case each frame
);

// Fires when a scale node is about to be created.
// Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodeScale,
    s16*  drawingLayer; // render priority layer (0–15)
    f32*  scale;        // scale factor (1.0 = normal)
    void** displayList; // optional attached display list (may be NULL)
);

// Fires when a shadow node is about to be created.
// Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodeShadow,
    u8*  shadowType;     // shadow rendering style (blob, projected, etc.)
    u8*  shadowSolidity; // shadow opacity (0–255)
    s16* shadowScale;    // shadow size multiplier relative to object
);

// Fires when a display-list node is about to be created.
// Cancellable — cancel to suppress node creation.
DEFINE_EVENT(GeoLayoutNodeDisplayList,
    s32*  drawingLayer; // render priority layer (0–15)
    void** displayList; // graphics command list to execute
);

// ────────────────────────────────────────────────────────────────────────────
// Entity distance culling
// ────────────────────────────────────────────────────────────────────────────

// Fires when the engine's distance check would cull an entity from loading.
// Write *visible = true or cancel to force the entity to load.
DEFINE_EVENT(EntityDistanceLoad,
    bool* visible; // write true to override culling and force load
);

// Fires when the engine's distance check would cull a loaded entity from rendering.
// Write *visible = true or cancel to force the entity to render.
DEFINE_EVENT(EntityDistanceRender,
    bool* visible; // write true to override culling and force render
);

// ────────────────────────────────────────────────────────────────────────────
// Behavior script
// ────────────────────────────────────────────────────────────────────────────

// Fires when a behavior script executes a native C function call. Cancellable.
// Cancel — suppresses the native function call.
DEFINE_EVENT(BehaviorCallNative,
    void (*function)(void); // the native function about to be called
);
