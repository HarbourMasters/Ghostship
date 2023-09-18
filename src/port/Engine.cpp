#include "Engine.h"
#include "ui/ImguiUI.h"
#include "port/importer/AnimationFactory.h"
#include "port/importer/AudioBankFactory.h"
#include "port/importer/AudioSampleFactory.h"
#include "port/importer/AudioSequenceFactory.h"
#include "audio/internal.h"
#include "banks_table.h"
#include "sequences_table.h"
#include "audio/GameAudio.h"
#include "ZAPDUtils/Utils/StringHelper.h"
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

extern "C" {
#include "audio/external.h"
}

GameEngine* GameEngine::Instance;

GameEngine::GameEngine(){
    std::vector<std::string> OTRFiles;
    std::string cubePath = LUS::Context::GetPathRelativeToAppDirectory("smcube.otr");
    if (std::filesystem::exists(cubePath)) {
        OTRFiles.push_back(cubePath);
    }
   std::string sohOtrPath = LUS::Context::GetPathRelativeToAppBundle("soh.otr");
   if (std::filesystem::exists(sohOtrPath)) {
       OTRFiles.push_back(sohOtrPath);
   }
    std::string patchesPath = LUS::Context::GetPathRelativeToAppDirectory("mods");
    if (patchesPath.length() > 0 && std::filesystem::exists(patchesPath)) {
        if (std::filesystem::is_directory(patchesPath)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(patchesPath)) {
                if (StringHelper::IEquals(p.path().extension().string(), ".otr")) {
                    OTRFiles.push_back(p.path().generic_string());
                }
            }
        }
    }
    this->context = LUS::Context::CreateInstance("Ghostship", "sm64", "ghostship.cfg.json", OTRFiles, {}, 3);
    this->context->GetWindow()->SetTargetFps(30);
    this->context->GetWindow()->SetMaximumFrameLatency(1);
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Anim, "Animation", std::make_shared<CubeOS::AnimationFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Bank, "Bank", std::make_shared<CubeOS::AudioBankFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Sample, "Sample", std::make_shared<CubeOS::AudioSampleFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Sequence, "Sequence", std::make_shared<CubeOS::AudioSequenceFactory>());
    GameEngine::AudioInit();
    uint32_t version = LUS::Context::GetInstance()->GetResourceManager()->GetArchive()->GetGameVersions()[0];
}

void GameEngine::Create(){
    GameEngine::Instance = new GameEngine();
    GameUI::SetupGuiElements();
}

void GameEngine::Destroy(){
    GameEngine::AudioExit();
    this->context = nullptr;
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

// Audio

void GameEngine::HandleAudioThread(){
    while (audio.running) {
        {
            std::unique_lock<std::mutex> Lock(audio.mutex);
            while (!audio.processing && audio.running) {
                audio.cv_to_thread.wait(Lock);
            }

            if (!audio.running) {
                break;
            }
        }
        std::unique_lock<std::mutex> Lock(audio.mutex);

        int samples_left = AudioPlayerBuffered();
        u32 num_audio_samples = samples_left < AudioPlayerGetDesiredBuffered() ? SAMPLES_HIGH : SAMPLES_LOW;

        s16 audio_buffer[SAMPLES_HIGH * NUM_AUDIO_CHANNELS * 3];
        for (int i = 0; i < AUDIO_FRAMES_PER_UPDATE; i++) {
            create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
        }

        AudioPlayerPlayFrame((u8 *) audio_buffer, 2 * num_audio_samples * 4);

        audio.processing = false;
        audio.cv_from_thread.notify_one();
    }
}

void GameEngine::StartAudioFrame(){
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.processing = true;
    }

    audio.cv_to_thread.notify_one();
}

void GameEngine::EndAudioFrame(){
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        while (audio.processing) {
            audio.cv_from_thread.wait(Lock);
        }
    }
}

void GameEngine::AudioInit() {
    LUS::Context::GetInstance()->GetResourceManager()->LoadDirectory("sound");

    if (!audio.running) {
        audio.running = true;
        audio.thread = std::thread(GameEngine::HandleAudioThread);
    }
}

void GameEngine::AudioExit() {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.running = false;
    }
    audio.cv_to_thread.notify_all();

    // Wait until the audio thread quit
    audio.thread.join();
}

extern "C" uint32_t GameEngine_GetSampleRate() {
    auto player = LUS::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (player == nullptr) {
        return 0;
    }

    if (!player->IsInitialized()) {
        return 0;
    }

    return player->GetSampleRate();
}

// End

extern "C" float GameEngine_GetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}

extern "C" CtlEntry* GameEngine_LoadBank(uint8_t bankId) {
    auto engine = GameEngine::Instance;
    if(bankId > (sizeof(gBankTable) / sizeof(gBankTable[0])) - 1){
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

extern "C" AudioSequenceData* GameEngine_LoadSequence(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    if(seqId > (sizeof(gSequenceTable) / sizeof(gSequenceTable[0])) - 1){
        return nullptr;
    }
    if(engine->sequences.contains(seqId)){
        return engine->sequences[seqId];
    }
    auto sequences = static_cast<AudioSequenceData *>(ResourceGetDataByName(gSequenceTable[seqId]));
    engine->sequences[seqId] = sequences;
    return sequences;
}

extern "C" uint8_t GameEngine_IsSequenceLoaded(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    GameEngine_LoadSequence(seqId);
    return engine->sequences.contains(seqId);
}

extern "C" void GameEngine_UnloadSequence(uint8_t seqId) {
    auto engine = GameEngine::Instance;
    if(engine->sequences.contains(seqId)){
        engine->sequences.erase(seqId);
    }
}

extern "C" uint32_t GameEngine_GetGameVersion() {
    return LUS::Context::GetInstance()->GetResourceManager()->GetArchive()->GetGameVersions()[0];
}