#ifndef BUFFERS_H
#define BUFFERS_H

#include <libultra/types.h>

#include "game/save_file.h"
#include "game/game_init.h"
#include "sm64_config.h"

#define SP_DRAM_STACK_SIZE8 0x400

extern_s u8 gDecompressionHeap[];

extern_s u8 gAudioHeap[];

extern_s u8 gAudioSPTaskYieldBuffer[];

extern_s u8 gUnusedThread2Stack[];

extern_s u8 gIdleThreadStack[];
extern_s u8 gThread3Stack[];
extern_s u8 gThread4Stack[];
extern_s u8 gThread5Stack[];
#if ENABLE_RUMBLE
extern_s u8 gThread6Stack[];
#endif

extern_s u8 gGfxSPTaskYieldBuffer[];

extern_s struct SaveBuffer gSaveBuffer;

extern_s u8 gGfxSPTaskStack[];

extern_s struct GfxPool gGfxPools[1];

#endif // BUFFERS_H
