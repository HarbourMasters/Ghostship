#include <libultraship.h>

#include <Fast3D/gfx_pc.h>
#include "Engine.h"

extern "C" {
#include "audio/external.h"
#include "game/game_init.h"
#include "sm64.h"
}

void alloc_pool() {
    static u64 pool[1024 * 1024 * 4];
    main_pool_init(pool, pool + sizeof(pool) / sizeof(pool[0]));
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);
}

void patch_interpolations(void) {
    mtx_patch_interpolated();
    patch_screen_transition_interpolated();
    patch_title_screen_scales();
    patch_interpolated_dialog();
    patch_interpolated_hud();
    patch_interpolated_paintings();
    patch_interpolated_bubble_particles();
    patch_interpolated_snow_particles();
}

#define ROUND_30(x) floor(x / 30) * 30

extern "C"
void exec_display_list(SPTask *spTask) {
    uint32_t targetFPS = GameEngine::GetInterpolationFPS();
    GameEngine::Instance->context->GetWindow()->SetTargetFps(targetFPS);

    int threshold = CVarGetInteger("gExtraLatencyThreshold", 80);
    GameEngine::Instance->context->GetWindow()->SetMaximumFrameLatency(threshold > 0 && targetFPS >= threshold ? 2 : 1);

    GameEngine::RunCommands((Gfx*) spTask->task.t.data_ptr);

    if (targetFPS > 30) {
        for (size_t i = 0; i < (ROUND_30(targetFPS) - 30) / 30; i++) {
            patch_interpolations();
            GameEngine::RunCommands((Gfx *) spTask->task.t.data_ptr);
        }
    }
}

void push_frame() {
    GameEngine::StartAudioFrame();
    GameEngine::Instance->StartFrame();
    thread5_iteration();
    GameEngine::EndAudioFrame();
}

#ifdef _WIN32
int SDL_main(int argc, char **argv) {
#else
int main(){
#endif
    GameEngine::Create();
    alloc_pool();
    audio_init();
    sound_init();
    thread5_game_loop();
    GameEngine::Instance->ProcessFrame(push_frame);
    GameEngine::Instance->Destroy();
    return 0;
}