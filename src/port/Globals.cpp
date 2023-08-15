#include "Globals.h"

#include <iostream>
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

GameEngine* GameEngine::Instance;

GameEngine::GameEngine(){
    std::cout << "GameEngine::GameEngine" << std::endl;
    std::string main = LUS::Context::GetPathRelativeToAppBundle("soh.otr");
    this->context = LUS::Context::CreateInstance("Super Mario 64", "sm64", "sm64.json", {main}, {}, 3);
    this->context->GetWindow()->SetTargetFps(30);
    this->context->GetWindow()->SetMaximumFrameLatency(1);
}

extern "C" {
void GameEngine_Create(void){
    GameEngine::Instance = new GameEngine();
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

}