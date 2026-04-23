#include "PortEnhancements.h"

#define INIT_EVENT_IDS

#include "sm64.h"
#include "game/level_update.h"
#include "port/events/Events.h"
#include "assets/bin/segment2.h"
#include "port/Rando/Rando.h"
#include "port/ShipUtils.h"

uint8_t textRand[] = { 0x1B, 0x0A, 0x17, 0x0D, 0xFF };
uint8_t textMarioRando[] = { 0x1B, 0x0A, 0x17, 0x0D, 0x18, 0x16, 0x12, 0x23, 0x0E, 0x1B, 0xFF };

static const Mtx matrix_patch_identity = {
    { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } }
};

// 0x020144B0 - 0x020144F0
static const Mtx matrix_patch_fullscreen = { { { 2.0f / SCREEN_WIDTH, 0.0f, 0.0f, 0.0f },
                                               { 0.0f, 2.0f / SCREEN_HEIGHT, 0.0f, 0.0f },
                                               { 0.0f, 0.0f, -1.0f, 0.0f },
                                               { -1.0f, -1.0f, -1.0f, 1.0f } } };

void PatchSetupDList() {
    Gfx identity = gsSPMatrix(&matrix_patch_identity, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    Gfx fullscreen = gsSPMatrix(&matrix_patch_fullscreen, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    Gfx model = gsSPMatrix(&matrix_patch_identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    Gfx nop = gsSPNoOp();

    // 0
    GfxPatch pt_mtx_fullscreen[] = { { 4, identity }, { 5, nop },   { 6, fullscreen },
                                     { 7, nop },      { 8, model }, { 9, nop } };
    ResourceMgr_PatchGfxByName(dl_proj_mtx_fullscreen, "SetupFullscreenProjMtx", pt_mtx_fullscreen,
                               ARRAY_COUNT(pt_mtx_fullscreen));

    // 1
    GfxPatch pt_skybox_begin[] = { { 6, identity }, { 7, nop } };
    ResourceMgr_PatchGfxByName(dl_skybox_begin, "SetupSkyboxBegin", pt_skybox_begin, ARRAY_COUNT(pt_skybox_begin));

    // 2
    GfxPatch pt_skybox_tile_settings[] = { { 0, model }, { 1, nop } };
    ResourceMgr_PatchGfxByName(dl_skybox_tile_tex_settings, "SetupSkyboxTileTexSettings", pt_skybox_tile_settings,
                               ARRAY_COUNT(pt_skybox_tile_settings));

    // 3
    GfxPatch pt_up_arrow[] = { { 7, identity }, { 8, nop } };
    ResourceMgr_PatchGfxByName(dl_ia8_up_arrow_begin, "SetupUpArrowBegin", pt_up_arrow, ARRAY_COUNT(pt_up_arrow));
}

void PortEnhancements_Init() {
    PortEnhancements_Register();
    PatchSetupDList();

    // Register event listeners
    REGISTER_LISTENER(PlayerHealthChange, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        PlayerHealthChange* ev = (PlayerHealthChange*)event;
        if (CVarGetInteger("gCheats.InfiniteHealth", 0) == 0 || ev->health > 0) {
            return;
        }

        event->Cancelled = true;
    });
    REGISTER_LISTENER(PlayerLivesChange, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        PlayerLivesChange* ev = (PlayerLivesChange*)event;
        if (CVarGetInteger("gCheats.InfiniteLives", 0) == 0 || ev->lives > 0) {
            return;
        }

        event->Cancelled = true;
    });
    REGISTER_LISTENER(RenderPauseCourseOptions, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (CVarGetInteger("gCheats.PauseExitWhenever", 0) == 0) {
            return;
        }

        RenderPauseCourseOptions* ev = (RenderPauseCourseOptions*)event;
        *ev->render = true;
    });

    REGISTER_LISTENER(LevelInitFromSaveFile, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (CVarGetInteger("gEnhancements.DisableLakituCutscene", 0)) {
            gNeverEnteredCastle = false;
        }
    });

    auto OnDistanceFunc = [](IEvent* event) {
        if (CVarGetInteger("gEnhancements.DisableDrawDistance", 0) == 0) {
            return;
        }

        EntityDistanceRender* ev = (EntityDistanceRender*)event;
        *ev->visible = true;
    };

    REGISTER_LISTENER(EntityDistanceLoad, EVENT_PRIORITY_NORMAL, OnDistanceFunc);
    REGISTER_LISTENER(EntityDistanceRender, EVENT_PRIORITY_NORMAL, OnDistanceFunc);
}

void PortEnhancements_Register() {
    // Register engine events
    REGISTER_EVENT(OnGameFileLoad);
    REGISTER_EVENT(OnGameFileSave);
    REGISTER_EVENT(GameFrameUpdate);
    REGISTER_EVENT(GameLoopTick);
    REGISTER_EVENT(GameReadInput);
    REGISTER_EVENT(BehaviorCallNative);
    REGISTER_EVENT(RenderHud);
    REGISTER_EVENT(RenderTextLabels);
    REGISTER_EVENT(RenderGamePre);
    REGISTER_EVENT(RenderGamePost);
    REGISTER_EVENT(GeoLayoutCallASM);
    REGISTER_EVENT(LevelScriptExecute);
    REGISTER_EVENT(EntityDistanceLoad);
    REGISTER_EVENT(LevelScriptCallLoop);
    REGISTER_EVENT(LevelScriptBeginArea);
    REGISTER_EVENT(EntityDistanceRender);
    REGISTER_EVENT(LevelInitFromSaveFile);
    REGISTER_EVENT(RenderPauseCourseOptions);

    // Register player events
    REGISTER_EVENT(PlayerHealthChange);
    REGISTER_EVENT(PlayerLivesChange);
    REGISTER_EVENT(PlayerStartedDialog);
    REGISTER_EVENT(PlayerDeath);
    REGISTER_EVENT(PlayerSetAction);
    REGISTER_EVENT(PlayerExecuteAction);
    REGISTER_EVENT(PlayerCheckCommonAirborneCancels);

    // Register Rando Events
    REGISTER_EVENT(ItemCollected);
    REGISTER_EVENT(MacroObjectOverride);
    REGISTER_EVENT(SpawnStar);
    REGISTER_EVENT(SpawnCoinStar);
    REGISTER_EVENT(ModifyDefaultStar);
    REGISTER_EVENT(ModifyObjectBehavior);
    REGISTER_EVENT(ModifyRedCoinCount);
    REGISTER_EVENT(ModifyObjectVisibility);
    REGISTER_EVENT(ChangeLevel);
    REGISTER_EVENT(ExitLevel);

    // Register game events
    REGISTER_EVENT(CapSwitchActivated);
    REGISTER_EVENT(ChainChompRelease);
    REGISTER_EVENT(BossDefeated);
    REGISTER_EVENT(SpawnCollectible);
    REGISTER_EVENT(BossBattleStarted);
    REGISTER_EVENT(BossBattleEnded);
    REGISTER_EVENT(MusicChanged);
    REGISTER_EVENT(GameEnded);

    Rando::Init();
    LoadGuiTextures();
}