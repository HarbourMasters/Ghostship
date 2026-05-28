#ifndef VARIABLES_H
#define VARIABLES_H

#include "macros.h"

#define BAD_PI 3.141592654
#define BAD_DTOR (BAD_PI/ 180.0)
#define M_TAU (2*M_PI)
#define BAD_TAU 6.2831853
#define M_PI 3.14159265358979323846

struct Overlay {
    void *start;
    void *end;
};

extern_s struct Overlay gOverlayTable[];

extern_s f32  climbPoleBottom[3];
extern_s f32  climbPoleTop[3];

extern_s const char gBuildVersion[];
extern_s const u16 gBuildVersionMajor;
extern_s const u16 gBuildVersionMinor;
extern_s const u16 gBuildVersionPatch;
extern_s const char gGitBranch[];
extern_s const char gGitCommitHash[];
extern_s u8 gGitCommitTag[];
#endif
