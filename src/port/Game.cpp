#include <libultraship.h>

#include <fast/interpreter.h>
#include "Engine.h"
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include "port/web/WebUtils.h"
#endif
#ifdef __ANDROID__
// The packaged game data is unpacked before this process starts rather than
// from here: extracting the ROM needs config.yml and assets/ymls already on
// disk. See android/app/src/main/java/com/ghostship/android/GameAssets.kt.
#include <SDL2/SDL_main.h>
#endif

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

extern "C" void exec_display_list(SPTask* spTask) {
    GameEngine::ProcessGfxCommands((Gfx*)spTask->task.t.data_ptr);
}

void push_frame() {
    GameEngine::StartAudioFrame();
    GameEngine::Instance->StartFrame();
    thread5_iteration();
    GameEngine::EndAudioFrame();
#ifdef __EMSCRIPTEN__
    static uint32_t lastSync = 0;
    const uint32_t now = SDL_GetTicks();
    if (now - lastSync > 5000) {
        lastSync = now;
        WebCache_Save();
    }
#endif
}

#ifdef _WIN32
int SDL_main(int argc, char** argv) {
#else
int main(int argc, char* argv[]) {
#endif
#ifdef __EMSCRIPTEN__
    WebCache_Mount("/storage");
    WebCache_Load();
#endif
    GameEngine::Create(argc, argv);
    alloc_pool();
    audio_init();
    sound_init();
    thread5_game_loop();
    while (WindowIsRunning()) {
        push_frame();
    }
    GameEngine::Instance->Destroy();
    return 0;
}