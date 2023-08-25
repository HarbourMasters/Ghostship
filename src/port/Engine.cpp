#include "Engine.h"
#include "ImguiUI.h"
#include "port/importer/AnimationFactory.h"

#include <iostream>
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

GameEngine* GameEngine::Instance;

GameEngine::GameEngine(){
    std::string main = LUS::Context::GetPathRelativeToAppBundle("smcube.otr");
    std::string assets = LUS::Context::GetPathRelativeToAppBundle("soh.otr");
    this->context = LUS::Context::CreateInstance("Ghostship", "sm64", "ghostship.cfg.json", { main, assets }, {}, 3);
    this->context->GetWindow()->SetTargetFps(30);
    this->context->GetWindow()->SetMaximumFrameLatency(1);
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Anim, "Animation", std::make_shared<CubeOS::AnimationFactory>());
}

void GameEngine::Create(){
    GameEngine::Instance = new GameEngine();
    GameUI::SetupGuiElements();
}

void GameEngine::StartFrame() const{
    this->context->GetWindow()->StartFrame();
}

void GameEngine::RunCommands(Gfx* Commands) {
    gfx_run(Commands, {});
    gfx_end_frame();
}

void GameEngine::ProcessFrame(void (*run_one_game_iter)()) const {
    this->context->GetWindow()->MainLoop(run_one_game_iter);
}

extern "C" uint32_t GameEngine_GetSampleRate() {
    auto audio = LUS::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->GetSampleRate();
}

extern "C" float GameEngine_GetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}