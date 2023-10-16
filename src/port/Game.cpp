#include <libultraship.h>

#include <Fast3D/gfx_pc.h>
#include "Engine.h"

extern "C" {
#include "audio/external.h"
#include "game/game_init.h"
#include "sm64.h"
float gInterpolationStep = 0.0f;
}

void alloc_pool() {
    static u64 pool[1024 * 1024 * 4];
    main_pool_init(pool, pool + sizeof(pool) / sizeof(pool[0]));
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);
}

void patch_interpolations() {
    mtx_patch_interpolated();
    patch_screen_transition_interpolated();
    patch_title_screen_scales();
    patch_interpolated_dialog();
    patch_interpolated_hud();
    patch_interpolated_paintings();
    patch_interpolated_bubble_particles();
    patch_interpolated_snow_particles();
}

extern "C"
void exec_display_list(SPTask *spTask) {
    int target_fps = GameEngine::GetInterpolationFPS();
    static int last_fps;
    static int time;
    int fps = target_fps;
    int original_fps = 30;

    if (target_fps == 30 || original_fps > target_fps) {
        fps = original_fps;
    }

    if (last_fps != fps) {
        time = 0;
    }

    int next_original_frame = fps;
    while (time + original_fps <= next_original_frame) {
        time += original_fps;
        if (time != next_original_frame) {
            gInterpolationStep = (float)time / next_original_frame;
        }
        GameEngine::RunCommands((Gfx *) spTask->task.t.data_ptr);
        patch_interpolations();
    }

    time -= fps;

    GameEngine::Instance->context->GetWindow()->SetTargetFps(fps);

    int threshold = CVarGetInteger("gExtraLatencyThreshold", 80);
    GameEngine::Instance->context->GetWindow()->SetMaximumFrameLatency(threshold > 0 && target_fps >= threshold ? 2 : 1);

    last_fps = fps;
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