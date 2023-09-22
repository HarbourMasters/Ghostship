#include <libultraship.h>

#include "sm64.h"
#include <Fast3D/gfx_pc.h>

extern "C" {
#include "game/main.h"
#include "game/game_init.h"
#include "segments.h"
s8 D_8032C650 = 0;
s8 gResetTimer = 0;
u32 gNumVblanks = 0;
s8 gNmiResetBarsTimer = 0;
u64 osClockRate = 62500000;

s8 gShowProfiler = FALSE;
s8 gShowDebugText = FALSE;
s8 gDebugLevelSelect = FALSE;

OSIoMesg gDmaIoMesg;
OSMesg gMainReceivedMesg;
OSMesgQueue gDmaMesgQueue;
OSMesgQueue gSIEventMesgQueue;
}