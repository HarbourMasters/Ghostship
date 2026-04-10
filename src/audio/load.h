#ifndef AUDIO_LOAD_H
#define AUDIO_LOAD_H

#include <libultra/types.h>

#include "internal.h"

#define AUDIO_FRAME_DMA_QUEUE_SIZE 0x40

#define PRELOAD_BANKS 2
#define PRELOAD_SEQUENCE 1

#define IS_SEQUENCE_CHANNEL_VALID(ptr) ((uintptr_t)(ptr) != (uintptr_t)&gSequenceChannelNone)

extern_s struct Note *gNotes;

// Music in SM64 is played using 3 players:
// gSequencePlayers[0] is level background music
// gSequencePlayers[1] is misc music, like the puzzle jingle
// gSequencePlayers[2] is sound
extern_s struct SequencePlayer gSequencePlayers[SEQUENCE_PLAYERS];

extern_s struct SequenceChannel gSequenceChannels[SEQUENCE_CHANNELS];
extern_s struct SequenceChannelLayer gSequenceLayers[SEQUENCE_LAYERS];

extern_s struct SequenceChannel gSequenceChannelNone;

extern_s struct AudioListItem gLayerFreeList;
extern_s struct NotePool gNoteFreeLists;

extern_s OSMesgQueue gCurrAudioFrameDmaQueue;
extern_s u32 gSampleDmaNumListItems;
extern_s s32 gAiFrequency;

#if defined(VERSION_EU) || defined(VERSION_SH)
extern_s struct AudioBufferParametersEU gAudioBufferParameters;
#endif
#ifdef VERSION_SH
extern_s s16 gCurrAiBufferLength;
extern_s s32 D_SH_8034F68C;
#endif
extern_s s32 gMaxAudioCmds;

extern_s s32 gMaxSimultaneousNotes;
extern_s s32 gSamplesPerFrameTarget;
extern_s s32 gMinAiBufferLength;
extern_s s16 gTempoInternalToExternal;
extern_s s8 gAudioUpdatesPerFrame; // = 4
extern_s s8 gSoundMode;

#ifdef VERSION_SH
extern_s OSMesgQueue gUnkQueue1;

struct UnkStructSH8034EC88 {
    u8 *endAndMediumIdentification;
    struct AudioBankSample *sample;
    u8 *ramAddr;
    u32 encodedInfo;
    s32 isFree;
};

struct PatchStruct {
    s32 bankId1;
    s32 bankId2;
    void *baseAddr1;
    void *baseAddr2;
    s32 medium1;
    s32 medium2;
};

extern_s struct UnkStructSH8034EC88 D_SH_8034EC88[0x80];
#endif

void audio_dma_partial_copy_async(uintptr_t *devAddr, u8 **vAddr, ssize_t *remaining, OSMesgQueue *queue, OSIoMesg *mesg);
void decrease_sample_dma_ttls(void);
#ifdef VERSION_SH
void *dma_sample_data(uintptr_t devAddr, u32 size, s32 arg2, u8 *dmaIndexRef, s32 medium);
#else
void *dma_sample_data(uintptr_t devAddr, u32 size, s32 arg2, u8 *dmaIndexRef);
#endif
void init_sample_dma_buffers(s32 arg0);
#if defined(VERSION_SH)
void patch_audio_bank(s32 bankId, struct AudioBank *mem, struct PatchStruct *patchInfo);
#else
void patch_audio_bank(struct AudioBank *mem, u8 *offset, u32 numInstruments, u32 numDrums);
#endif
#ifdef VERSION_SH
void preload_sequence(u32 seqId, s32 preloadMask);
#else
void preload_sequence(u32 seqId, u8 preloadMask);
#endif
void load_sequence(u32 player, u32 seqId, s32 loadAsync);

#ifdef VERSION_SH
void func_sh_802f3158(s32 seqId, s32 arg1, s32 arg2, OSMesgQueue *retQueue);
u8 *func_sh_802f3220(u32 seqId, u32 *a1);
struct AudioBankSample *func_sh_802f4978(s32 bankId, s32 idx);
s32 func_sh_802f47c8(s32 bankId, u8 idx, s8 *io);
void *func_sh_802f3f08(s32 poolIdx, s32 arg1, s32 arg2, s32 arg3, OSMesgQueue *retQueue);
void func_sh_802f41e4(s32 audioResetStatus);
BAD_RETURN(s32) func_sh_802f3368(s32 bankId);
void *func_sh_802f3764(s32 arg0, s32 idx, s32 *arg2);
s32 func_sh_802f3024(s32 bankId, s32 instId, s32 arg2);
void func_sh_802f30f4(s32 arg0, s32 arg1, s32 arg2, OSMesgQueue *arg3);
void func_sh_802f3288(s32 idx);

#endif

#endif // AUDIO_LOAD_H
