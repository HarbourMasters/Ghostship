#include <libultraship.h>

#include "sm64.h"
#include <Fast3D/gfx_pc.h>
#include "Globals.h"

#define SAMPLES_HIGH 752
#define SAMPLES_LOW 720
#define AUDIO_FRAMES_PER_UPDATE 2
#define NUM_AUDIO_CHANNELS 2

extern "C" {
#include "game/main.h"
#include "game/game_init.h"
#include "audio/external.h"
#include "segments.h"
s8 D_8032C650 = 0;
s8 gResetTimer = 0;
u32 gNumVblanks = 0;
s8 gNmiResetBarsTimer = 0;
u64 osClockRate = 62500000;

s8 sAudioEnabled = TRUE;
s8 gShowProfiler = FALSE;
s8 gShowDebugText = FALSE;
s8 gDebugLevelSelect = TRUE;

OSIoMesg gDmaIoMesg;
OSMesg gMainReceivedMesg;
OSMesgQueue gDmaMesgQueue;
OSMesgQueue gSIEventMesgQueue;
}

extern "C" {
void set_vblank_handler(s32 index, VblankHandler *handler, OSMesgQueue *queue, OSMesg msg) {}
void dispatch_audio_sptask(SPTask *spTask) {}

void alloc_pool(void) {
    static u64 pool[1024 * 1024 * 4];
    main_pool_init(pool, pool + sizeof(pool) / sizeof(pool[0]));
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);
}

void exec_display_list(SPTask *spTask) {
    if(gDisplayListHead == nullptr){
        return;
    }
    GameEngine_RunCommands((Gfx*) spTask->task.t.data_ptr);
}
}

void push_frame(void) {
    GameEngine_StartFrame();
    thread5_iteration();

    if(sAudioEnabled) {
        int samples_left = AudioPlayerBuffered();
        u32 num_audio_samples = samples_left < AudioPlayerGetDesiredBuffered() ? SAMPLES_HIGH : SAMPLES_LOW;

        s16 audio_buffer[SAMPLES_HIGH * NUM_AUDIO_CHANNELS * 2];
        for (int i = 0; i < AUDIO_FRAMES_PER_UPDATE; i++) {
            create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
        }

        AudioPlayerPlayFrame((u8 *) audio_buffer, 2 * num_audio_samples * 4);
    }
}

int main(){
    GameEngine_Create();
    alloc_pool();
    audio_init();
    sound_init();

    thread5_game_loop(NULL);
    GameEngine_ProcessFrame(push_frame);
}