#include "Globals.h"
#include "ImguiUI.h"

#include <iostream>
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

GameEngine* GameEngine::Instance;

GameEngine::GameEngine(){
    std::cout << "GameEngine::GameEngine" << std::endl;
    std::string main = LUS::Context::GetPathRelativeToAppBundle("soh.otr");
    this->context = LUS::Context::CreateInstance("Ghostship", "sm64", "ghostship.cfg.json", {main}, {}, 3);
    this->context->GetWindow()->SetTargetFps(30);
    this->context->GetWindow()->SetMaximumFrameLatency(1);
}

void GameEngine_Create(void){
    GameEngine::Instance = new GameEngine();
    GameUI::SetupGuiElements();
}

void GameEngine_StartFrame(void){
    GameEngine::Instance->context->GetWindow()->StartFrame();
}

void GameEngine_RunCommands(Gfx* Commands) {
    gfx_run(Commands, {});
    gfx_end_frame();
}

void GameEngine_ProcessFrame(void (*run_one_game_iter)(void)) {
    GameEngine::Instance->context->GetWindow()->MainLoop(run_one_game_iter);
}

extern "C" uint32_t GameEngine_GetSampleRate(void) {
    auto audio = LUS::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->GetSampleRate();
}