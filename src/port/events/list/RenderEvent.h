#pragma once

#include "include/types.h"
#include "game/area.h"

// ────────────────────────────────────────────────────────────────────────────
// HUD elements — all cancellable
// Cancel any of these to suppress that element entirely for the current frame.
// ────────────────────────────────────────────────────────────────────────────

// Fires before the lives counter is rendered.
DEFINE_EVENT(RenderHudLives,
    s16 lives; // current displayed lives count
);

// Fires before the coin counter is rendered.
DEFINE_EVENT(RenderHudCoins,
    s16 coins; // current displayed coin count
);

// Fires before the star counter is rendered.
DEFINE_EVENT(RenderHudStars,
    s16 stars; // current displayed star count
);

// Fires before the power meter (health wedges) is rendered.
DEFINE_EVENT(RenderHudPowerMeter,
    s16 wedges; // number of health wedges currently shown
);

// Fires before the camera mode icon is rendered.
DEFINE_EVENT(RenderHudCameraStatus,
    s16 status; // current camera status flags (CAM_STATUS_*)
);

// Fires before the slide timer is rendered.
DEFINE_EVENT(RenderHudTimer,
    u16 timer; // raw frame count for the slide timer
);

// ────────────────────────────────────────────────────────────────────────────
// Background / environment rendering — all cancellable
// ────────────────────────────────────────────────────────────────────────────

// Fires in geo_skybox_main during GEO_CONTEXT_RENDER before the skybox is drawn.
// Write *background to substitute a different skybox image ID.
// Cancel — suppresses the skybox entirely (sky will be black).
DEFINE_EVENT(SkyboxRender,
    s8* background; // mutable skybox image index (BACKGROUND_*)
);

// Fires in geo_movtex_draw_nocolor and geo_movtex_draw_colored during GEO_CONTEXT_RENDER.
// Write *movtexId to redirect to a different moving-texture group.
// Cancel — suppresses the water/lava/sand surface rendering.
DEFINE_EVENT(MovingTextureRender,
    s16* movtexId; // mutable moving-texture group ID (matches gMovtex*[].geoId)
);

// ────────────────────────────────────────────────────────────────────────────
// Screen transitions — all cancellable
// ────────────────────────────────────────────────────────────────────────────

// Fires in render_fade_transition_from_color before the color fade-out overlay is drawn.
// Write through transData->red/green/blue to change the overlay color.
// Cancel — suppresses the overlay entirely.
DEFINE_EVENT(ScreenTransitionFadeOut,
    struct WarpTransitionData* transData; // mutable transition parameters
    u8 alpha;                             // current overlay opacity (0–255)
);

// Fires in render_fade_transition_into_color before the color fade-in overlay is drawn.
// Write through transData fields to change color. Cancel — suppresses the overlay.
DEFINE_EVENT(ScreenTransitionFadeIn,
    struct WarpTransitionData* transData; // mutable transition parameters
    u8 alpha;                             // current overlay opacity (0–255)
);

// Fires in render_textured_transition (star/circle/Mario/Bowser wipe) before rendering.
// Write *texId to substitute a different wipe texture (TEX_TRANS_*).
// Cancel — suppresses the wipe transition entirely.
DEFINE_EVENT(ScreenTransitionTexture,
    struct WarpTransitionData* transData; // mutable transition parameters
    s8* texId;                            // mutable texture index (TEX_TRANS_*)
);

// ────────────────────────────────────────────────────────────────────────────
// Menu observation events (not cancellable — use Override variants to replace)
// ────────────────────────────────────────────────────────────────────────────

// Fires each frame in bhv_act_selector_loop. Use to overlay custom UI on the
// act selection screen without replacing it entirely.
DEFINE_EVENT(StarSelectRender,
    s16 courseNum;   // current course number (COURSE_*)
    u8  stars;       // bitmask of collected stars in this course
    s8  selectedAct; // currently highlighted act (0-based)
);

// ────────────────────────────────────────────────────────────────────────────
// Menu override events — all cancellable
// Cancel any of these to suppress the default menu rendering entirely and draw
// a custom replacement using ImGui or the RCP display list in your listener.
// ────────────────────────────────────────────────────────────────────────────

// Fires in geo_file_select_strings_and_menu_cursor during GEO_CONTEXT_RENDER.
// Cancel — suppresses default file-select strings and cursor rendering.
DEFINE_EVENT(FileSelectOverride);

// Fires in geo_act_selector_strings during GEO_CONTEXT_RENDER.
// Cancel — suppresses default act-name label rendering.
DEFINE_EVENT(StarSelectOverride,
    s16 courseNum;   // current course number (COURSE_*)
    u8  stars;       // bitmask of collected stars in this course
    s8  selectedAct; // currently highlighted act (0-based)
);

// Fires at the top of render_pause_courses_and_castle().
// menuMode is MENU_MODE_RENDER_PAUSE_SCREEN or MENU_MODE_UNUSED_0.
// Cancel — skips all pause rendering; returns MENU_OPT_NONE.
DEFINE_EVENT(PauseMenuOverride,
    s16 menuMode; // current gMenuMode value (MENU_MODE_*)
);

// Fires at the top of render_course_complete_screen().
// Cancel — skips the star/coin results screen; returns MENU_OPT_NONE.
DEFINE_EVENT(CourseCompleteOverride,
    s16 courseNum; // course that was just completed (COURSE_*)
);

// Fires at the top of render_dialog_entries() when a dialog box is about to render.
// Cancel — suppresses the default dialog box.
DEFINE_EVENT(DialogOverride,
    s16 dialogId; // current dialog ID (DIALOG_*)
);
