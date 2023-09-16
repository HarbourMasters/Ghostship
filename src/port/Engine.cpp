#include "Engine.h"
#include "ImguiUI.h"
#include "port/importer/AnimationFactory.h"
#include "port/importer/AudioBankFactory.h"
#include "port/importer/AudioSampleFactory.h"
#include "audio/internal.h"
#include "banks_table.h"
#include "sequences_table.h"

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
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Bank, "Bank", std::make_shared<CubeOS::AudioBankFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Audio, "Audio", std::make_shared<CubeOS::AudioSampleFactory>());
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

extern "C" CtlEntry* GameEngine_LoadBank(uint8_t bankId) {
    auto engine = GameEngine::Instance;
    if(bankId > (sizeof(gBankTable) / sizeof(gBankTable[0]))){
        return nullptr;
    }
    if(engine->banks.contains(bankId)){
        return engine->banks[bankId];
    }
    auto ctl = static_cast<CtlEntry *>(ResourceGetDataByName(gBankTable[bankId]));
    engine->banks[bankId] = ctl;
    return ctl;
}

extern "C" uint8_t GameEngine_IsBankLoaded(uint8_t bankId) {
    auto engine = GameEngine::Instance;
    GameEngine_LoadBank(bankId);
    return engine->banks.contains(bankId);
}

extern "C" void GameEngine_UnloadBank(uint8_t bankId) {
    auto engine = GameEngine::Instance;
    if(engine->banks.contains(bankId)){
        engine->banks.erase(bankId);
    }
}

extern "C" AudioBankSample* GameEngine_LoadSequence(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    if(seqId > (sizeof(gSequenceTable) / sizeof(gSequenceTable[0]))){
        return nullptr;
    }
    if(engine->samples.contains(seqId)){
        return engine->samples[seqId];
    }
    auto sample = static_cast<AudioBankSample *>(ResourceGetDataByName(gSequenceTable[seqId]));
    engine->samples[seqId] = sample;
    return sample;
}

extern "C" uint8_t GameEngine_IsSequenceLoaded(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    GameEngine_LoadSequence(seqId);
    return engine->samples.contains(seqId);
}

extern "C" void GameEngine_UnloadSequence(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    if(engine->samples.contains(seqId)){
        engine->samples.erase(seqId);
    }
}