#pragma once

#include "types.h"

typedef struct {
    /* 0x00 */ PrintCallback callback;
    /* 0x04 */ Gfx* dList;
    /* 0x08 */ u16 posX;
    /* 0x0A */ u16 posY;
    /* 0x0C */ u16 baseX;
    /* 0x0E */ u8 baseY;
    /* 0x0F */ u8 flags;
    /* 0x10 */ Color_RGBA8_u32 color;
    /* 0x14 */ char unk_14[0x1C]; // unused
} GfxPrint; // size = 0x30

#define GFXP_UNUSED "\x8E"
#define GFXP_UNUSED_CHAR 0x8E
#define GFXP_HIRAGANA "\x8D"
#define GFXP_HIRAGANA_CHAR 0x8D
#define GFXP_KATAKANA "\x8C"
#define GFXP_KATAKANA_CHAR 0x8C
#define GFXP_RAINBOW_ON "\x8B"
#define GFXP_RAINBOW_ON_CHAR 0x8B
#define GFXP_RAINBOW_OFF "\x8A"
#define GFXP_RAINBOW_OFF_CHAR 0x8A

#define GFXP_FLAG_HIRAGANA (1 << 0)
#define GFXP_FLAG_RAINBOW  (1 << 1)
#define GFXP_FLAG_SHADOW   (1 << 2)
#define GFXP_FLAG_UPDATE   (1 << 3)
#define GFXP_FLAG_ENLARGE  (1 << 6)
#define GFXP_FLAG_OPEN     (1 << 7)
#define G_LOAD_UCODE 0xDD

#define gSPLoadUcode(pkt, uc_index)                                             \
    _DW({                                                                       \
        Gfx* _g = (Gfx*)(pkt);                                                  \
        _g->words.w0 = _SHIFTL(G_LOAD_UCODE, 24, 8) | _SHIFTL(uc_index, 0, 16); \
    })

extern_s void GfxPrint_SetColor(GfxPrint* self, u32 r, u32 g, u32 b, u32 a);
extern_s void GfxPrint_SetPosPx(GfxPrint* self, s32 x, s32 y);
extern_s void GfxPrint_SetPos(GfxPrint* self, s32 x, s32 y);
extern_s void GfxPrint_SetBasePosPx(GfxPrint* self, s32 x, s32 y);
extern_s void GfxPrint_Init(GfxPrint* self);
extern_s void GfxPrint_Destroy(GfxPrint* self);
extern_s void GfxPrint_Open(GfxPrint* self, Gfx* dList);
extern_s Gfx* GfxPrint_Close(GfxPrint* self);
extern_s s32 GfxPrint_Printf(GfxPrint* self, const char* fmt, ...);