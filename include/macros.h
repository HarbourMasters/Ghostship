#ifndef MACROS_H
#define MACROS_H

#include "platform_info.h"
#include <ship/Api.h>

#ifndef __sgi
#define GLOBAL_ASM(...)
#endif

#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))

#define GLUE(a, b) a ## b
#define GLUE2(a, b) GLUE(a, b)

#ifdef _WIN32
#ifndef __DLL__
#define extern_s API_EXTERN __declspec(dllexport)
#else
#define extern_s API_EXTERN __declspec(dllimport)
#endif
#else
#define extern_s API_EXTERN
#endif

// Avoid compiler warnings for unused variables
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

// Avoid undefined behaviour for non-returning functions
#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

// Static assertions
#ifdef __GNUC__
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond, msg) typedef char GLUE2(static_assertion_failed, __LINE__)[(cond) ? 1 : -1]
#endif

// Align to 8-byte boundary for DMA requirements
#ifdef __GNUC__
#define ALIGNED8 __attribute__((aligned(8)))
#else
#define ALIGNED8
#endif

// Align to 16-byte boundary for audio lib requirements
#ifdef __GNUC__
#define ALIGNED16 __attribute__((aligned(16)))
#else
#define ALIGNED16
#endif

#define ROUND_UP_64(v) (((v) + 63) & ~63)
#define ROUND_UP_32(v) (((v) + 31) & ~31)
#define ROUND_UP_16(v) (((v) + 15) & ~15)
#define ROUND_UP_8(v) (((v) + 7) & ~7)
#define ROUND_DOWN_16(v) ((v) & ~0xf)

// no conversion needed other than cast
#define VIRTUAL_TO_PHYSICAL(addr)   ((uintptr_t)(addr))
#define PHYSICAL_TO_VIRTUAL(addr)   ((uintptr_t)(addr))
#define VIRTUAL_TO_PHYSICAL2(addr)  ((void *)(addr))

#define	G_CC_PRIMITIVE              0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define	G_CC_SHADE                  0, 0, 0, SHADE, 0, 0, 0, SHADE

#define	G_CC_MODULATEI              TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define	G_CC_MODULATEIDECALA        TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0
#define	G_CC_MODULATEIFADE          TEXEL0, 0, SHADE, 0, 0, 0, 0, ENVIRONMENT

#define	G_CC_MODULATERGB            G_CC_MODULATEI
#define	G_CC_MODULATERGBDECALA      G_CC_MODULATEIDECALA
#define	G_CC_MODULATERGBFADE        G_CC_MODULATEIFADE

#define	G_CC_MODULATEIA             TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0
#define	G_CC_MODULATEIFADEA         TEXEL0, 0, SHADE, 0, TEXEL0, 0, ENVIRONMENT, 0

#define	G_CC_MODULATEFADE           TEXEL0, 0, SHADE, 0, ENVIRONMENT, 0, TEXEL0, 0

#define	G_CC_MODULATERGBA           G_CC_MODULATEIA
#define	G_CC_MODULATERGBFADEA       G_CC_MODULATEIFADEA

#define	G_CC_MODULATEI_PRIM         TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define	G_CC_MODULATEIA_PRIM        TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define	G_CC_MODULATEIDECALA_PRIM   TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0

#define	G_CC_MODULATERGB_PRIM       G_CC_MODULATEI_PRIM
#define	G_CC_MODULATERGBA_PRIM      G_CC_MODULATEIA_PRIM
#define	G_CC_MODULATERGBDECALA_PRIM G_CC_MODULATEIDECALA_PRIM

#define	G_CC_FADE                   SHADE, 0, ENVIRONMENT, 0, SHADE, 0, ENVIRONMENT, 0
#define	G_CC_FADEA                  TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0

#define	G_CC_DECALRGB               0, 0, 0, TEXEL0, 0, 0, 0, SHADE
#define	G_CC_DECALRGBA              0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0
#define	G_CC_DECALFADE              0, 0, 0, TEXEL0, 0, 0, 0, ENVIRONMENT

#define	G_CC_DECALFADEA             0, 0, 0, TEXEL0, TEXEL0, 0, ENVIRONMENT, 0

#define	G_CC_BLENDI                 ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDIA                ENVIRONMENT, SHADE, TEXEL0, SHADE, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDIDECALA           ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0

#define	G_CC_BLENDRGBA              TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDRGBDECALA         TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, TEXEL0
#define	G_CC_BLENDRGBFADEA          TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, ENVIRONMENT

#define G_CC_ADDRGB                 TEXEL0, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_ADDRGBDECALA           TEXEL0, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_ADDRGBFADE             TEXEL0, 0, TEXEL0, SHADE, 0, 0, 0, ENVIRONMENT

#define G_CC_REFLECTRGB             ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_REFLECTRGBDECALA       ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0

#define G_CC_HILITERGB              PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_HILITERGBA             PRIMITIVE, SHADE, TEXEL0, SHADE, PRIMITIVE, SHADE, TEXEL0, SHADE
#define G_CC_HILITERGBDECALA        PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0

#define G_CC_SHADEDECALA            0, 0, 0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_SHADEFADEA             0, 0, 0, SHADE, 0, 0, 0, ENVIRONMENT

#define	G_CC_BLENDPE                PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDPEDECALA          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, TEXEL0

#define	RM_AA_ZB_XLU_SURF(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

/* Custom version of RM_AA_ZB_XLU_SURF with Z_UPD */
#define RM_CUSTOM_AA_ZB_XLU_SURF(clk)				\
	RM_AA_ZB_XLU_SURF(clk) | Z_UPD

#define G_RM_CUSTOM_AA_ZB_XLU_SURF	RM_CUSTOM_AA_ZB_XLU_SURF(1)
#define G_RM_CUSTOM_AA_ZB_XLU_SURF2	RM_CUSTOM_AA_ZB_XLU_SURF(2)

#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1)	\
{									\
	gSP1Triangle(pkt, v00, v01, v02, flag0);			\
	gSP1Triangle(pkt, v10, v11, v12, flag1);			\
}
#define gsSP2Triangles(v00, v01, v02, flag0, v10, v11, v12, flag1)	\
	gsSP1Triangle(v00, v01, v02, flag0),				\
	gsSP1Triangle(v10, v11, v12, flag1)

/*
 * gsSPGeometryMode
 * In Fast3DEX2 it is better to use this, as the RSP geometry mode
 * is able to be set and cleared in a single command.
 */
#define gsSPGeometryMode(c, s)						\
	gsSPClearGeometryMode(c),					\
	gsSPSetGeometryMode(s)
#define gsSPGeometryModeSetFirst(c, s)					\
	gsSPSetGeometryMode(s),						\
	gsSPClearGeometryMode(c)

#define gDPLoadTextureBlockWide(pkt, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)          \
    _DW({                                                                                                             \
        gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);                                                      \
        gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts);       \
        gDPLoadSync(pkt);                                                                                             \
        gDPLoadBlockWide(pkt, G_TX_LOADTILE, 0, 0, (((width) * (height) + siz##_INCR) >> siz##_SHIFT) - 1,                \
                     CALC_DXT(width, siz##_BYTES));                                                                   \
        gDPPipeSync(pkt);                                                                                             \
        gDPSetTile(pkt, fmt, siz, (((width)*siz##_LINE_BYTES) + 7) >> 3, 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, \
                   cms, masks, shifts);                                                                               \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC,                               \
                       ((height)-1) << G_TEXTURE_IMAGE_FRAC);                                                         \
    })

#endif // MACROS_H
