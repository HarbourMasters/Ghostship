#include "MirrorMode.h"

#include <libultraship.h>
#include "../../ui/cvar_prefixes.h"
#include "sm64.h"
#include "game/game_init.h"

#define CVAR_MIRROR_MODE CVAR_ENHANCEMENT("Modes.MirroredWorld.Mode")

static int sMirrorEnabled = 0;
static int sMirrorActive = 0;
static int sCounterMirror = 0;
static int sExcludeForFrame = 0;

int mirror_mode_is_enabled(void) {
    return CVarGetInteger(CVAR_MIRROR_MODE, 0);
}

int mirror_mode_is_active(void) {
    return sMirrorActive;
}

void mirror_mode_apply_projection(void) {
    if (!mirror_mode_is_enabled()) {
        return;
    }

    // Check if we should exclude mirroring for this frame
    if (sExcludeForFrame) {
        sExcludeForFrame = 0;
        return;
    }

    Mtx* mtx = (Mtx*)alloc_display_list(sizeof(Mtx));
    if (mtx == NULL) {
        return;
    }

    // Apply horizontal scale flip
    guScale(mtx, -1.0f, 1.0f, 1.0f);
    gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx), G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);

    // Set invert culling flag for proper face sorting
    gSPSetExtraGeometryMode(gDisplayListHead++, G_EX_INVERT_CULLING);

    sMirrorActive = 1;
}

void mirror_mode_undo_projection(void) {
    if (!sMirrorActive) {
        return;
    }

    // Clear invert culling flag before HUD renders
    gSPClearExtraGeometryMode(gDisplayListHead++, G_EX_INVERT_CULLING);

    sMirrorActive = 0;
}

void mirror_mode_register(void) {
    // Events will be registered in mirror_mode_init via PortEnhancements
}

void mirror_mode_init(void) {
    // Update enabled state from CVAR
    sMirrorEnabled = mirror_mode_is_enabled();
}

void mirror_mode_set_counter_mirror(void) {
    sCounterMirror = 1;
}

void mirror_mode_clear_counter_mirror(void) {
    sCounterMirror = 0;
}

int mirror_mode_should_counter_mirror(void) {
    return sCounterMirror && sMirrorActive;
}

// Exclude mirroring for the current frame (for file select, etc.)
void mirror_mode_exclude_for_frame(void) {
    sExcludeForFrame = 1;
}

// Apply counter-mirror scale to cancel projection mirror for text/numbers
void mirror_mode_apply_counter_scale(void) {
    if (!mirror_mode_should_counter_mirror()) {
        return;
    }

    Mtx* mtx = (Mtx*)alloc_display_list(sizeof(Mtx));
    if (mtx == NULL) {
        return;
    }

    // Apply counter-scale to cancel the projection mirror
    guScale(mtx, -1.0f, 1.0f, 1.0f);
    gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
}

// Patch display lists to add counter-mirror for number sprites
void mirror_mode_patch_number_dl(void) {
    // This function would patch the MODEL_NUMBER display list
    // to include a counter-mirror transform at the beginning
    // However, in the PC port, display lists are loaded dynamically
    // and the patching would need to happen at runtime
    //
    // The approach would be:
    // 1. Load the number_geo display list
    // 2. Inject a counter-mirror matrix at the beginning
    // 3. This would make all number sprites appear unmirrored
    //
    // This is currently a placeholder for future implementation
    // as it requires deeper integration with the display list system
}

void mirror_mode_invert_input(void) {
    if (!mirror_mode_is_enabled()) {
        return;
    }

    struct Controller* ctrl = gPlayer1Controller;
    if (ctrl == NULL) {
        return;
    }

    // Invert BOTH rawStickX and processed stickX
    // rawStickX is used for some purposes, but stickX is what mario.c uses for movement
    ctrl->rawStickX = -ctrl->rawStickX;
    ctrl->stickX = -ctrl->stickX;

    // Also need to update magnitude
    ctrl->stickMag = sqrtf(ctrl->stickX * ctrl->stickX + ctrl->stickY * ctrl->stickY);
}
