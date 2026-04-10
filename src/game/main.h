#ifndef MAIN_H
#define MAIN_H

#include <libultraship.h>
#include "sm64_config.h"

struct RumbleData {
    u8 unk00;
    u8 unk01;
    s16 unk02;
    s16 unk04;
};

struct StructSH8031D9B0 {
    s16 unk00;
    s16 unk02;
    s16 unk04;
    s16 unk06;
    s16 unk08;
    s16 unk0A;
    s16 unk0C;
    s16 unk0E;
};

extern_s OSThread D_80339210;
extern_s OSThread gIdleThread;
extern_s OSThread gMainThread;
extern_s OSThread gGameLoopThread;
extern_s OSThread gSoundThread;
#if ENABLE_RUMBLE
extern_s OSThread gRumblePakThread;

extern_s OSPfs gRumblePakPfs; // Actually an OSPfs but we don't have that header yet
#endif

extern_s OSMesgQueue gPIMesgQueue;
extern_s OSMesgQueue gIntrMesgQueue;
extern_s OSMesgQueue gSPTaskMesgQueue;
#if ENABLE_RUMBLE
extern_s OSMesgQueue gRumblePakSchedulerMesgQueue;
extern_s OSMesgQueue gRumbleThreadVIMesgQueue;
#endif
extern_s OSMesg gDmaMesgBuf[1];
extern_s OSMesg gPIMesgBuf[32];
extern_s OSMesg gSIEventMesgBuf[1];
extern_s OSMesg gIntrMesgBuf[16];
extern_s OSMesg gUnknownMesgBuf[16];
extern_s OSIoMesg gDmaIoMesg;
extern_s OSMesg gMainReceivedMesg;
extern_s OSMesgQueue gDmaMesgQueue;
extern_s OSMesgQueue gSIEventMesgQueue;
#if ENABLE_RUMBLE
extern_s OSMesg gRumblePakSchedulerMesgBuf[1];
extern_s OSMesg gRumbleThreadVIMesgBuf[1];

extern_s struct RumbleData gRumbleDataQueue[3];
extern_s struct StructSH8031D9B0 gCurrRumbleSettings;
#endif

extern_s struct VblankHandler *gVblankHandler1;
extern_s struct VblankHandler *gVblankHandler2;
extern_s struct SPTask *gActiveSPTask;
extern_s u32 gNumVblanks;
extern_s s8 gResetTimer;
extern_s s8 gNmiResetBarsTimer;
extern_s s8 D_8032C650;
extern_s s8 gShowProfiler;
extern_s s8 gShowDebugText;

#define gDebugLevelSelect (CVarGetInteger("gDeveloperTools.DebugMode", 0) == 1)

void exec_display_list(struct SPTask *spTask);

#endif // MAIN_H
