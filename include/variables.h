#ifndef VARIABLES_H
#define VARIABLES_H

#define BAD_PI 3.141592654
#define BAD_DTOR (BAD_PI/ 180.0)
#define M_TAU (2*M_PI)
#define BAD_TAU 6.2831853
#define M_PI 3.14159265358979323846

struct Overlay {
    void *start;
    void *end;
};

extern struct Overlay gOverlayTable[];

extern f32  climbPoleBottom[3];
extern f32  climbPoleTop[3];

extern const char gBuildVersion[];
extern const u16 gBuildVersionMajor;
extern const u16 gBuildVersionMinor;
extern const u16 gBuildVersionPatch;
extern const char gGitBranch[];
extern const char gGitCommitHash[];
extern u8 gGitCommitTag[];
#endif
