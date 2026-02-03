#include <libultra/types.h>
#include "osAi.h"
#include "macros.h"

extern s32 osViClock;

s32 osAiSetFrequency(u32 freq) {
    u32 a1;
    s32 a2;
    u32 D_8033491C;

#ifdef VERSION_EU
    D_8033491C = 0x02E6025C;
#else
    D_8033491C = 0x02E6D354;
#endif

    a1 = D_8033491C / (float) freq + .5f;

    if (a1 < 0x84) {
        return -1;
    }

    a2 = (a1 / 66) & 0xff;
    if (a2 > 16) {
        a2 = 16;
    }

    return D_8033491C / (s32) a1;
}


#ifndef VERSION_SH
// put some extra jr $ra's down there please
UNUSED static void filler1(void) {
}

UNUSED static void filler2(void) {
}
#endif
