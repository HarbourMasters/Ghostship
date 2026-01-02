#include "PortEnhancements.h"
#include "sm64.h"
#include "game/level_update.h"

#define INIT_EVENT_IDS
#include "port/hooks/Events.h"

#include "assets/bin/segment2.h"

#define MARIO_HEALTH_MAX 0x880

static const Mtx matrix_patch_identity = {{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
}};


// 0x020144B0 - 0x020144F0
static const Mtx matrix_patch_fullscreen = {{
    { 2.0f / SCREEN_WIDTH, 0.0f, 0.0f, 0.0f },
    { 0.0f, 2.0f / SCREEN_HEIGHT, 0.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f }
}};

void OnGameUpdate(IEvent* event) {
    if(gMarioState == NULL) {
        return;
    }

    if (CVarGetInteger("gInfiniteLives", 0) != 0) {
        gMarioState->numLives = MARIO_HEALTH_MAX;
    }
}

void OnHealthChange(IEvent* event) {
    if (CVarGetInteger("gInfiniteHealth", 0) != 0) {
        gMarioState->health = MARIO_HEALTH_MAX;
    }
}

void PatchSetupDList() {
    Gfx identity = gsSPMatrix(&matrix_patch_identity, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    Gfx fullscreen = gsSPMatrix(&matrix_patch_fullscreen, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    Gfx model = gsSPMatrix(&matrix_patch_identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    Gfx nop = gsSPNoOp();

    // 0
    GfxPatch pt_mtx_fullscreen[] = {
        { 4, identity },
        { 5, nop },
        { 6, fullscreen },
        { 7, nop },
        { 8, model },
        { 9, nop }
    };
    ResourceMgr_PatchGfxByName(dl_proj_mtx_fullscreen, "SetupFullscreenProjMtx", pt_mtx_fullscreen, ARRAY_COUNT(pt_mtx_fullscreen));

    // 1
    GfxPatch pt_skybox_begin[] = {
        { 6, identity },
        { 7, nop }
    };
    ResourceMgr_PatchGfxByName(dl_skybox_begin, "SetupSkyboxBegin", pt_skybox_begin, ARRAY_COUNT(pt_skybox_begin));
    
    // 2
    GfxPatch pt_skybox_tile_settings[] = {
        { 0, model },
        { 1, nop }
    };
    ResourceMgr_PatchGfxByName(dl_skybox_tile_tex_settings, "SetupSkyboxTileTexSettings", pt_skybox_tile_settings, ARRAY_COUNT(pt_skybox_tile_settings));

    // 3
    GfxPatch pt_up_arrow[] = {
        { 7, identity },
        { 8, nop }
    };
    ResourceMgr_PatchGfxByName(dl_ia8_up_arrow_begin, "SetupUpArrowBegin", pt_up_arrow, ARRAY_COUNT(pt_up_arrow));
}

void PortEnhancements_Init() {
    PortEnhancements_Register();
    PatchSetupDList();

    // Register event listeners
    REGISTER_LISTENER(GameFrameUpdate, OnGameUpdate, EVENT_PRIORITY_NORMAL);
    REGISTER_LISTENER(PlayerHealthChange, OnHealthChange, EVENT_PRIORITY_NORMAL);
}

void PortEnhancements_Register() {
    // Register engine events
    REGISTER_EVENT(GameFrameUpdate);
    REGISTER_EVENT(PlayerHealthChange);
}

void PortEnhancements_Exit() {
    // TODO: Unregister event listeners
}
