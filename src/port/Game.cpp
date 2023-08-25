#include <libultraship.h>

#include <Fast3D/gfx_pc.h>
#include "Engine.h"

extern "C" {
#include "audio/external.h"
#include "game/game_init.h"
}

bool sAudioEnabled = false;

void alloc_pool() {
    static u64 pool[1024 * 1024 * 4];
    main_pool_init(pool, pool + sizeof(pool) / sizeof(pool[0]));
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);
}

extern "C"
void exec_display_list(SPTask *spTask) {
    GameEngine::RunCommands((Gfx*) spTask->task.t.data_ptr);
}

void push_frame() {
    GameEngine::Instance->StartFrame();
    thread5_iteration();

    if(sAudioEnabled) {
        int samples_left = AudioPlayerBuffered();
        u32 num_audio_samples = samples_left < AudioPlayerGetDesiredBuffered() ? SAMPLES_HIGH : SAMPLES_LOW;

        s16 audio_buffer[SAMPLES_HIGH * NUM_AUDIO_CHANNELS * 3];
        for (int i = 0; i < AUDIO_FRAMES_PER_UPDATE; i++) {
            create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
        }

        AudioPlayerPlayFrame((u8 *) audio_buffer, 2 * num_audio_samples * 4);
    }
}

int main(){
    GameEngine::Create();
    alloc_pool();
    audio_init();
    sound_init();

    thread5_game_loop(nullptr);
    GameEngine::Instance->ProcessFrame(push_frame);
}