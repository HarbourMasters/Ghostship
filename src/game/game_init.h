#ifndef GAME_INIT_H
#define GAME_INIT_H

#include <libultraship.h>

#include "types.h"
#include "memory.h"

#define GFX_POOL_SIZE 6400 // Size of how large the master display list (gDisplayListHead) can be

struct GfxPool {
    Gfx buffer[GFX_POOL_SIZE];
    struct SPTask spTask;
};

struct DemoInput {
    u8 timer; // time until next input. if this value is 0, it means the demo is over
    s8 rawStickX;
    s8 rawStickY;
    u8 buttonMask;
};

extern_s struct Controller gControllers[3];
extern_s OSContStatus gControllerStatuses[4];
extern_s OSContPad gControllerPads[4];
extern_s OSMesgQueue gGameVblankQueue;
extern_s OSMesgQueue gGfxVblankQueue;
extern_s OSMesg gGameMesgBuf[1];
extern_s OSMesg gGfxMesgBuf[1];
extern_s struct VblankHandler gGameVblankHandler;
extern_s uintptr_t gPhysicalFramebuffers[3];
extern_s uintptr_t gPhysicalZBuffer;
extern_s void *gMarioAnimsMemAlloc;
extern_s struct SPTask *gGfxSPTask;
extern_s Gfx *gDisplayListHead;
extern_s u8 *gGfxPoolEnd;
extern_s struct GfxPool *gGfxPool;
extern_s u8 gControllerBits;
extern_s s8 gEepromProbe;

extern_s void (*gGoddardVblankCallback)(void);
extern_s struct Controller *gPlayer1Controller;
extern_s struct Controller *gPlayer2Controller;
extern_s struct Controller *gPlayer3Controller;
extern_s struct DemoInput *gCurrDemoInput;
extern_s u16 gDemoInputListID;
extern_s struct DemoInput gRecordedDemoInput;

extern_s u16 sRenderingFramebuffer;
extern_s u32 gGlobalTimer;

extern_s void setup_game_memory(void);
extern_s void thread5_game_loop(void);
extern_s void clear_framebuffer(s32 color);
extern_s void clear_viewport(Vp *viewport, s32 color);
extern_s void make_viewport_clip_rect(Vp *viewport);
extern_s void init_rcp(void);
extern_s void end_master_display_list(void);
extern_s void render_init(void);
extern_s void select_gfx_pool(void);
extern_s void display_and_vsync(void);
extern_s void thread5_iteration(void);

#endif // GAME_INIT_H
