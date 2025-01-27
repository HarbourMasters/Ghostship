#ifndef VERSION_SH
#include <libultraship.h>

#include "sm64.h"
#include "data.h"
#include "external.h"
#include "heap.h"
#include "load.h"
#include "seqplayer.h"
#include "sound/sound_data.h"
#include <string.h>
#include <stdlib.h>

#define ALIGN16(val) (((val) + 0xF) & ~0xF)

struct SharedDma {
    /*0x0*/ u8 *buffer;       // target, points to pre-allocated buffer
    /*0x4*/ uintptr_t source; // device address
    /*0x8*/ u16 sizeUnused;   // set to bufSize, never read
    /*0xA*/ u16 bufSize;      // size of buffer
    /*0xC*/ u8 unused2;       // set to 0, never read
    /*0xD*/ u8 reuseIndex;    // position in sSampleDmaReuseQueue1/2, if ttl == 0
    /*0xE*/ u8 ttl;           // duration after which the DMA can be discarded
};                            // size = 0x10

// EU only
void port_eu_init(void);

struct Note *gNotes;

struct SequencePlayer gSequencePlayers[SEQUENCE_PLAYERS];
struct SequenceChannel gSequenceChannels[SEQUENCE_CHANNELS];
struct SequenceChannelLayer gSequenceLayers[SEQUENCE_LAYERS];

struct SequenceChannel gSequenceChannelNone;
struct AudioListItem gLayerFreeList;
struct NotePool gNoteFreeLists;

OSMesgQueue gCurrAudioFrameDmaQueue;
OSMesg gCurrAudioFrameDmaMesgBufs[AUDIO_FRAME_DMA_QUEUE_SIZE];
OSIoMesg gCurrAudioFrameDmaIoMesgBufs[AUDIO_FRAME_DMA_QUEUE_SIZE];

OSMesgQueue gAudioDmaMesgQueue;
OSMesg gAudioDmaMesg;
OSIoMesg gAudioDmaIoMesg;

struct SharedDma sSampleDmas[0x60];
u32 gSampleDmaNumListItems; // sh: 0x803503D4
u32 sSampleDmaListSize1; // sh: 0x803503D8
u32 sUnused80226B40; // set to 0, never read, sh: 0x803503DC

// Circular buffer of DMAs with ttl = 0. tail <= head, wrapping around mod 256.
u8 sSampleDmaReuseQueue1[256];
u8 sSampleDmaReuseQueue2[256];
u8 sSampleDmaReuseQueueTail1;
u8 sSampleDmaReuseQueueTail2;
u8 sSampleDmaReuseQueueHead1; // sh: 0x803505E2
u8 sSampleDmaReuseQueueHead2; // sh: 0x803505E3

// bss correct up to here

u16 gSequenceCount;

#if defined(VERSION_EU)
u32 padEuBss1;
struct AudioBufferParametersEU gAudioBufferParameters;
#endif

u32 sDmaBufSize;
s32 gMaxAudioCmds;
s32 gMaxSimultaneousNotes;

#if defined(VERSION_EU)
s16 gTempoInternalToExternal;
#else
s32 gSamplesPerFrameTarget;
s32 gMinAiBufferLength;

s16 gTempoInternalToExternal;

s8 gAudioUpdatesPerFrame;
#endif

s8 gSoundMode;

#if defined(VERSION_EU)
s8 gAudioUpdatesPerFrame;
#endif

extern u64 gAudioGlobalsStartMarker;
extern u64 gAudioGlobalsEndMarker;

ALSeqFile *get_audio_file_header(s32 arg0);

#include <stdio.h>
/**
 * Performs an immediate DMA copy
 */
void audio_dma_copy_immediate(uintptr_t devAddr, void *vAddr, size_t nbytes) {
    eu_stubbed_printf_3("Romcopy %x -> %x ,size %x\n", devAddr, vAddr, nbytes);
    osInvalDCache(vAddr, nbytes);
    osPiStartDma(&gAudioDmaIoMesg, OS_MESG_PRI_HIGH, OS_READ, devAddr, vAddr, nbytes,
                 &gAudioDmaMesgQueue);
    osRecvMesg(&gAudioDmaMesgQueue, NULL, OS_MESG_NOBLOCK);
    eu_stubbed_printf_0("Romcopyend\n");
}

#ifdef VERSION_EU
u8 audioString34[] = "CAUTION:WAVE CACHE FULL %d";
u8 audioString35[] = "BASE %x %x\n";
u8 audioString36[] = "LOAD %x %x %x\n";
u8 audioString37[] = "INSTTOP    %x\n";
u8 audioString38[] = "INSTMAP[0] %x\n";
u8 audioString39[] = "already flags %d\n";
u8 audioString40[] = "already flags %d\n";
u8 audioString41[] = "ERR:SLOW BANK DMA BUSY\n";
u8 audioString42[] = "ERR:SLOW DMA BUSY\n";
u8 audioString43[] = "Check %d  bank %d\n";
u8 audioString44[] = "Cache Check\n";
u8 audioString45[] = "NO BANK ERROR\n";
u8 audioString46[] = "BANK %d LOADING START\n";
u8 audioString47[] = "BANK %d LOAD MISS (NO MEMORY)!\n";
u8 audioString48[] = "BANK %d ALREADY CACHED\n";
u8 audioString49[] = "BANK LOAD MISS! FOR %d\n";
#endif

/**
 * Performs an asynchronus (normal priority) DMA copy
 */
void audio_dma_copy_async(uintptr_t devAddr, void *vAddr, size_t nbytes, OSMesgQueue *queue, OSIoMesg *mesg) {
    osInvalDCache(vAddr, nbytes);
    osPiStartDma(mesg, OS_MESG_PRI_NORMAL, OS_READ, devAddr, vAddr, nbytes, queue);
}

/**
 * Performs a partial asynchronous (normal priority) DMA copy. This is limited
 * to 0x1000 bytes transfer at once.
 */
void audio_dma_partial_copy_async(uintptr_t *devAddr, u8 **vAddr, ssize_t *remaining, OSMesgQueue *queue, OSIoMesg *mesg) {
#if defined(VERSION_EU)
    ssize_t transfer = (*remaining >= 0x1000 ? 0x1000 : *remaining);
#else
    ssize_t transfer = (*remaining < 0x1000 ? *remaining : 0x1000);
#endif
    *remaining -= transfer;
    osInvalDCache(*vAddr, transfer);
    osPiStartDma(mesg, OS_MESG_PRI_NORMAL, OS_READ, *devAddr, *vAddr, transfer, queue);
    *devAddr += transfer;
    *vAddr += transfer;
}

void decrease_sample_dma_ttls() {
    u32 i;

    for (i = 0; i < sSampleDmaListSize1; i++) {
#if defined(VERSION_EU)
        struct SharedDma *temp = &sSampleDmas[i];
#else
        struct SharedDma *temp = sSampleDmas + i;
#endif
        if (temp->ttl != 0) {
            temp->ttl--;
            if (temp->ttl == 0) {
                temp->reuseIndex = sSampleDmaReuseQueueHead1;
                sSampleDmaReuseQueue1[sSampleDmaReuseQueueHead1++] = (u8) i;
            }
        }
    }

    for (i = sSampleDmaListSize1; i < gSampleDmaNumListItems; i++) {
#if defined(VERSION_EU)
        struct SharedDma *temp = &sSampleDmas[i];
#else
        struct SharedDma *temp = sSampleDmas + i;
#endif
        if (temp->ttl != 0) {
            temp->ttl--;
            if (temp->ttl == 0) {
                temp->reuseIndex = sSampleDmaReuseQueueHead2;
                sSampleDmaReuseQueue2[sSampleDmaReuseQueueHead2++] = (u8) i;
            }
        }
    }

    sUnused80226B40 = 0;
}

void *dma_sample_data(uintptr_t devAddr, u32 size, s32 arg2, u8 *dmaIndexRef) {
    s32 hasDma = FALSE;
    struct SharedDma *dma;
    uintptr_t dmaDevAddr;
    u32 transfer;
    u32 i;
    u32 dmaIndex;
    ssize_t bufferPos;
    UNUSED u32 pad;

    if (arg2 != 0 || *dmaIndexRef >= sSampleDmaListSize1) {
        for (i = sSampleDmaListSize1; i < gSampleDmaNumListItems; i++) {
            dma = &sSampleDmas[i];
            bufferPos = devAddr - dma->source;
            if (0 <= bufferPos && (size_t) bufferPos <= dma->bufSize - size) {
                // We already have a DMA request for this memory range.
                if (dma->ttl == 0 && sSampleDmaReuseQueueTail2 != sSampleDmaReuseQueueHead2) {
                    // Move the DMA out of the reuse queue, by swapping it with the
                    // tail, and then incrementing the tail.
                    if (dma->reuseIndex != sSampleDmaReuseQueueTail2) {
                        sSampleDmaReuseQueue2[dma->reuseIndex] =
                            sSampleDmaReuseQueue2[sSampleDmaReuseQueueTail2];
                        sSampleDmas[sSampleDmaReuseQueue2[sSampleDmaReuseQueueTail2]].reuseIndex =
                            dma->reuseIndex;
                    }
                    sSampleDmaReuseQueueTail2++;
                }
                dma->ttl = 60;
                *dmaIndexRef = (u8) i;
                return &dma->buffer[(devAddr - dma->source)];
            }
        }

        if (sSampleDmaReuseQueueTail2 != sSampleDmaReuseQueueHead2 && arg2 != 0) {
            // Allocate a DMA from reuse queue 2. This queue can be empty, since
            // TTL 60 is pretty large.
            dmaIndex = sSampleDmaReuseQueue2[sSampleDmaReuseQueueTail2];
            sSampleDmaReuseQueueTail2++;
            dma = sSampleDmas + dmaIndex;
            hasDma = TRUE;
        }
    } else {
#if defined(VERSION_EU)
        dma = sSampleDmas;
        dma += *dmaIndexRef;
#else
        dma = sSampleDmas + *dmaIndexRef;
#endif
        bufferPos = devAddr - dma->source;
        if (0 <= bufferPos && (size_t) bufferPos <= dma->bufSize - size) {
            // We already have DMA for this memory range.
            if (dma->ttl == 0) {
                // Move the DMA out of the reuse queue, by swapping it with the
                // tail, and then incrementing the tail.
                if (dma->reuseIndex != sSampleDmaReuseQueueTail1) {
#if defined(VERSION_EU)
                    if (1) {
                    }
#endif
                    sSampleDmaReuseQueue1[dma->reuseIndex] =
                        sSampleDmaReuseQueue1[sSampleDmaReuseQueueTail1];
                    sSampleDmas[sSampleDmaReuseQueue1[sSampleDmaReuseQueueTail1]].reuseIndex =
                        dma->reuseIndex;
                }
                sSampleDmaReuseQueueTail1++;
            }
            dma->ttl = 2;
#if defined(VERSION_EU)
            return dma->buffer + (devAddr - dma->source);
#else
            return (devAddr - dma->source) + dma->buffer;
#endif
        }
    }

    if (!hasDma) {
        // Allocate a DMA from reuse queue 1. This queue will hopefully never
        // be empty, since TTL 2 is so small.
        dmaIndex = sSampleDmaReuseQueue1[sSampleDmaReuseQueueTail1++];
        dma = sSampleDmas + dmaIndex;
        hasDma = TRUE;
    }

    transfer = dma->bufSize;
    dmaDevAddr = devAddr & ~0xF;
    dma->ttl = 2;
    dma->source = dmaDevAddr;
    dma->sizeUnused = transfer;
#ifdef VERSION_US
    osInvalDCache(dma->buffer, transfer);
#endif
#if defined(VERSION_EU)
    osPiStartDma(&gCurrAudioFrameDmaIoMesgBufs[gCurrAudioFrameDmaCount++], OS_MESG_PRI_NORMAL,
                     OS_READ, dmaDevAddr, dma->buffer, transfer, &gCurrAudioFrameDmaQueue);
    *dmaIndexRef = dmaIndex;
    return (devAddr - dmaDevAddr) + dma->buffer;
#else
    gCurrAudioFrameDmaCount++;
    osPiStartDma(&gCurrAudioFrameDmaIoMesgBufs[gCurrAudioFrameDmaCount - 1], OS_MESG_PRI_NORMAL,
                 OS_READ, dmaDevAddr, dma->buffer, transfer, &gCurrAudioFrameDmaQueue);
    *dmaIndexRef = dmaIndex;
    return (devAddr - dmaDevAddr) + dma->buffer;
#endif
}


void init_sample_dma_buffers(UNUSED s32 arg0) {
    s32 i;
#if defined(VERSION_EU)
#define j i
#else
    s32 j;
#endif

#if defined(VERSION_EU)
    sDmaBufSize = 0x400;
#else
    sDmaBufSize = 0x400;
#endif

#if defined(VERSION_EU)
    for (i = 0; i < gMaxSimultaneousNotes * 3 * gAudioBufferParameters.presetUnk4; i++)
#else
    for (i = 0; i < gMaxSimultaneousNotes * 3; i++)
#endif
    {
        sSampleDmas[gSampleDmaNumListItems].buffer = soundAlloc(&gNotesAndBuffersPool, sDmaBufSize);
        if (sSampleDmas[gSampleDmaNumListItems].buffer == NULL) {
#if defined(VERSION_EU)
            break;
#else
            goto out1;
#endif
        }
        sSampleDmas[gSampleDmaNumListItems].bufSize = sDmaBufSize;
        sSampleDmas[gSampleDmaNumListItems].source = 0;
        sSampleDmas[gSampleDmaNumListItems].sizeUnused = 0;
        sSampleDmas[gSampleDmaNumListItems].unused2 = 0;
        sSampleDmas[gSampleDmaNumListItems].ttl = 0;
        gSampleDmaNumListItems++;
    }
#if defined(VERSION_JP) || defined(VERSION_US)
out1:
#endif

    for (i = 0; (u32) i < gSampleDmaNumListItems; i++) {
        sSampleDmaReuseQueue1[i] = (u8) i;
        sSampleDmas[i].reuseIndex = (u8) i;
    }

    for (j = gSampleDmaNumListItems; j < 0x100; j++) {
        sSampleDmaReuseQueue1[j] = 0;
    }

    sSampleDmaReuseQueueTail1 = 0;
    sSampleDmaReuseQueueHead1 = (u8) gSampleDmaNumListItems;
    sSampleDmaListSize1 = gSampleDmaNumListItems;

#if defined(VERSION_EU)
    sDmaBufSize = 0x200;
#else
    sDmaBufSize = 0x200;
#endif
    for (i = 0; i < gMaxSimultaneousNotes; i++) {
        sSampleDmas[gSampleDmaNumListItems].buffer = soundAlloc(&gNotesAndBuffersPool, sDmaBufSize);
        if (sSampleDmas[gSampleDmaNumListItems].buffer == NULL) {
#if defined(VERSION_EU)
            break;
#else
            goto out2;
#endif
        }
        sSampleDmas[gSampleDmaNumListItems].bufSize = sDmaBufSize;
        sSampleDmas[gSampleDmaNumListItems].source = 0;
        sSampleDmas[gSampleDmaNumListItems].sizeUnused = 0;
        sSampleDmas[gSampleDmaNumListItems].unused2 = 0;
        sSampleDmas[gSampleDmaNumListItems].ttl = 0;
        gSampleDmaNumListItems++;
    }
#if defined(VERSION_JP) || defined(VERSION_US)
out2:
#endif

    for (i = sSampleDmaListSize1; (u32) i < gSampleDmaNumListItems; i++) {
        sSampleDmaReuseQueue2[i - sSampleDmaListSize1] = (u8) i;
        sSampleDmas[i].reuseIndex = (u8)(i - sSampleDmaListSize1);
    }

    // This probably meant to touch the range size1..size2 as well... but it
    // doesn't matter, since these values are never read anyway.
    for (j = gSampleDmaNumListItems; j < 0x100; j++) {
        sSampleDmaReuseQueue2[j] = sSampleDmaListSize1;
    }

    sSampleDmaReuseQueueTail2 = 0;
    sSampleDmaReuseQueueHead2 = gSampleDmaNumListItems - sSampleDmaListSize1;
#if defined(VERSION_EU)
#undef j
#endif
}

uint8_t* load_sequence_immediate(s32 seqId, s32 arg1) {
    return GameEngine_LoadSequence(seqId)->data;
}

struct CtlEntry* load_banks_immediate(s32 seqId, u8 *outDefaultBank) {
    u32 bankId;
    struct AudioSequenceData *seqData = GameEngine_LoadSequence(seqId);
    struct CtlEntry *output;
    for(size_t i = 0; i < seqData->bankCount; i++) {
        output = GameEngine_LoadBank(bankId = seqData->banks[i]);
    }
    *outDefaultBank = bankId;
    return output;
}

void preload_sequence(u32 seqId, u8 preloadMask) {
    u8 temp;

    if (seqId >= gSequenceCount) {
        return;
    }

    gAudioLoadLock = AUDIO_LOCK_LOADING;
    if (preloadMask & PRELOAD_BANKS) {
        // TODO: Bank is loaded on demand
        load_banks_immediate(seqId, &temp);
    }

    if (preloadMask & PRELOAD_SEQUENCE) {
        uint8_t* sequenceData = load_sequence_immediate(seqId, 2);
        if (sequenceData == NULL) {
            gAudioLoadLock = AUDIO_LOCK_NOT_LOADING;
            return;
        }
    }

    gAudioLoadLock = AUDIO_LOCK_NOT_LOADING;
}

void load_sequence_internal(u32 player, u32 seqId, s32 loadAsync);

void load_sequence(u32 player, u32 seqId, s32 loadAsync) {
    if (!loadAsync) {
        gAudioLoadLock = AUDIO_LOCK_LOADING;
    }
    load_sequence_internal(player, seqId, loadAsync);
    if (!loadAsync) {
        gAudioLoadLock = AUDIO_LOCK_NOT_LOADING;
    }
}

void load_sequence_internal(u32 player, u32 seqId, s32 loadAsync) {
    struct SequencePlayer *seqPlayer = &gSequencePlayers[player];

    if (seqId >= gSequenceCount) {
        return;
    }

    sequence_player_disable(seqPlayer);
    struct CtlEntry* bank = load_banks_immediate(seqId, &seqPlayer->defaultBank[0]);

    if (bank == NULL) {
        return;
    }

    eu_stubbed_printf_2("Seq %d:Default Load Id is %d\n", seqId, seqPlayer->defaultBank[0]);
    eu_stubbed_printf_0("Seq Loading Start\n");

    uint8_t* sequenceData = load_sequence_immediate(seqId, 2);
    if (sequenceData == NULL) {
        return;
    }

    seqPlayer->seqId = seqId;
    init_sequence_player(player);
    seqPlayer->scriptState.depth = 0;
    seqPlayer->delay = 0;
    seqPlayer->enabled = TRUE;
    seqPlayer->seqData = sequenceData;
    seqPlayer->scriptState.pc = sequenceData;
}

// (void) must be omitted from parameters to fix stack with -framepointer
void audio_init() {
    s32 i, j;
    UNUSED s32 lim1; // lim1 unused in EU
    UNUSED u32 size;
    UNUSED u64 *ptr64;
    UNUSED s32 pad2;

    gAudioLoadLock = AUDIO_LOCK_UNINITIALIZED;

#if defined(VERSION_JP) || defined(VERSION_US)
    lim1 = gUnusedCount80333EE8;
    for (i = 0; i < lim1; i++) {
        gUnused80226E58[i] = 0;
        gUnused80226E98[i] = 0;
    }

    memset(gAudioHeap, 0, gAudioHeapSize);
#else
    for (i = 0; i < gAudioHeapSize / 8; i++) {
        ((u64 *) gAudioHeap)[i] = 0;
    }


    D_EU_802298D0 = 20.03042f;
    gRefreshRate = 50;
    port_eu_init();
    if (k) {
    }
#endif

    eu_stubbed_printf_1("AudioHeap is %x\n", gAudioHeapSize);

    for (i = 0; i < NUMAIBUFFERS; i++) {
        gAiBufferLengths[i] = 0xa0;
    }

    gAudioFrameCount = 0;
    gAudioTaskIndex = 0;
    gCurrAiBufferIndex = 0;
    gSoundMode = 0;
    gAudioTask = NULL;
    gAudioTasks[0].task.t.data_size = 0;
    gAudioTasks[1].task.t.data_size = 0;
    osCreateMesgQueue(&gAudioDmaMesgQueue, &gAudioDmaMesg, 1);
    osCreateMesgQueue(&gCurrAudioFrameDmaQueue, gCurrAudioFrameDmaMesgBufs,
                      ARRAY_COUNT(gCurrAudioFrameDmaMesgBufs));
    gCurrAudioFrameDmaCount = 0;
    gSampleDmaNumListItems = 0;

    sound_init_main_pools(gAudioInitPoolSize);

    for (i = 0; i < NUMAIBUFFERS; i++) {
        gAiBuffers[i] = soundAlloc(&gAudioInitPool, AIBUFFER_LEN);

        for (j = 0; j < (s32) (AIBUFFER_LEN / sizeof(s16)); j++) {
            gAiBuffers[i][j] = 0;
        }
    }

#if defined(VERSION_EU)
    gAudioResetPresetIdToLoad = 0;
    gAudioResetStatus = 1;
    audio_shut_down_and_reset_step();
#else
    struct AudioSessionSettings* settings = ROM_JP ? &gAudioSessionPresetsJP[0] : &gAudioSessionPresetsUS[0];
    audio_reset_session(settings);
#endif

    // Load headers for sounds and sequences
    gSequenceCount = GameEngine_GetSequenceCount();
    init_sequence_players();
    gAudioLoadLock = AUDIO_LOCK_NOT_LOADING;

    audio_set_player_volume(SEQ_PLAYER_LEVEL, CVarGetFloat("gMainMusicVolume", 1.0f));
    audio_set_player_volume(SEQ_PLAYER_ENV, CVarGetFloat("gEnvironmentVolume", 1.0f));
    audio_set_player_volume(SEQ_PLAYER_SFX, CVarGetFloat("gSFXMusicVolume", 1.0f));

    // Should probably contain the sizes of the data banks, but those aren't
    // easily accessible from here.
    eu_stubbed_printf_0("---------- Init Completed. ------------\n");
    eu_stubbed_printf_1(" Syndrv    :[%6d]\n", 0); // gSoundDataADSR
    eu_stubbed_printf_1(" Seqdrv    :[%6d]\n", 0); // gMusicData
    eu_stubbed_printf_1(" audiodata :[%6d]\n", 0); // gSoundDataRaw
    eu_stubbed_printf_0("---------------------------------------\n");
}
#endif
