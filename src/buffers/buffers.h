#ifndef BUFFERS_H
#define BUFFERS_H

#include <libultra/types.h>

#include "game/save_file.h"
#include "game/game_init.h"
#include "config.h"

#define SP_DRAM_STACK_SIZE8 0x400

extern u8 gDecompressionHeap[];

extern u8 gAudioHeap[];

extern u8 gAudioSPTaskYieldBuffer[];

extern u8 gUnusedThread2Stack[];

extern u8 gIdleThreadStack[];
extern u8 gThread3Stack[];
extern u8 gThread4Stack[];
extern u8 gThread5Stack[];
#if ENABLE_RUMBLE
extern u8 gThread6Stack[];
#endif

extern u8 gGfxSPTaskYieldBuffer[];

extern struct SaveBuffer gSaveBuffer;

extern u8 gGfxSPTaskStack[];

extern struct GfxPool gGfxPools[1];

#endif // BUFFERS_H
