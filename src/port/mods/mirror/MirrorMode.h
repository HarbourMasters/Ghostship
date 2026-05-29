#pragma once

#include <libultra/gbi.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core state access
int mirror_mode_is_enabled(void);
int mirror_mode_is_active(void);

// Projection manipulation
void mirror_mode_apply_projection(void);
void mirror_mode_undo_projection(void);

// Counter-mirror for text-bearing elements (title, star doors, etc.)
void mirror_mode_set_counter_mirror(void);
void mirror_mode_clear_counter_mirror(void);
int mirror_mode_should_counter_mirror(void);

// Apply counter-mirror scale to cancel projection mirror for text/numbers
void mirror_mode_apply_counter_scale(void);

// Exclude mirroring for the current frame (for file select, etc.)
void mirror_mode_exclude_for_frame(void);

// Patch display lists for number sprites and star doors
void mirror_mode_patch_number_dl(void);

// Event registration
void mirror_mode_register(void);
void mirror_mode_init(void);

// Input inversion for mirror mode controls
void mirror_mode_invert_input(void);

#ifdef __cplusplus
}
#endif
