#include "Engine.h"
#include "ui/ImguiUI.h"
#include "port/importer/AnimationFactory.h"
#include "port/importer/AudioBankFactory.h"
#include "port/importer/AudioSampleFactory.h"
#include "port/importer/AudioSequenceFactory.h"
#include "audio/GameAudio.h"
#include "ZAPDUtils/Utils/StringHelper.h"
#include "texts_table.h"
#include "port/importer/DialogFactory.h"
#include "port/importer/DictionaryFactory.h"
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

#include <utility>

extern "C" {
#include "audio/external.h"
#include "audio/internal.h"
#include "game/ingame_menu.h"
}

GameEngine* GameEngine::Instance;

GameEngine::GameEngine(){
    std::vector<std::string> OTRFiles;
    std::string cubePath = LUS::Context::GetPathRelativeToAppDirectory("smcube.otr");
    if (std::filesystem::exists(cubePath)) {
        OTRFiles.push_back(cubePath);
    }
   std::string sm64OtrPath = LUS::Context::GetPathRelativeToAppBundle("sm64.otr");
   if (std::filesystem::exists(sm64OtrPath)) {
       OTRFiles.push_back(sm64OtrPath);
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
    this->context = LUS::Context::CreateInstance("Ghostship", "sm64", "ghostship.cfg.json", OTRFiles, { 0xFF2B5A63 }, 3);
    this->context->GetWindow()->SetTargetFps(60);
    this->context->GetWindow()->SetMaximumFrameLatency(1);
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::SAnim, "Animation", std::make_shared<CubeOS::AnimationFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Bank, "Bank", std::make_shared<CubeOS::AudioBankFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Sample, "Sample", std::make_shared<CubeOS::AudioSampleFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Sequence, "Sequence", std::make_shared<CubeOS::AudioSequenceFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::SDialog, "Dialog", std::make_shared<CubeOS::DialogFactory>());
    this->context->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(LUS::ResourceType::Dictionary, "Dictionary", std::make_shared<CubeOS::DictionaryFactory>());
}

void GameEngine::Create(){
    auto instance = GameEngine::Instance = new GameEngine();
    GameUI::SetupGuiElements();
    instance->AudioInit();
    instance->LoadDictionary();
}

void GameEngine::Destroy(){
    GameEngine::AudioExit();
}

bool ShouldClearTextureCacheAtEndOfFrame = false;

void GameEngine::StartFrame() const{
    using LUS::KbScancode;
    int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
        case KbScancode::LUS_KB_TAB: {
            // Toggle HD Assets
            CVarSetInteger("gAltAssets", !CVarGetInteger("gAltAssets", 0));
            ShouldClearTextureCacheAtEndOfFrame = true;
            break;
        }
    }
    this->context->GetWindow()->StartFrame();
}

void GameEngine::RunCommands(Gfx* Commands) {
    gfx_run(Commands, {});
    gfx_end_frame();

    if (ShouldClearTextureCacheAtEndOfFrame) {
        gfx_texture_cache_clear();
        ShouldClearTextureCacheAtEndOfFrame = false;
    }
}

void GameEngine::ProcessFrame(void (*run_one_game_iter)()) const {
    this->context->GetWindow()->MainLoop(run_one_game_iter);
}

uint32_t GameEngine::GetInterpolationFPS() {
    if (LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() == LUS::WindowBackend::DX11) {
        return CVarGetInteger("gInterpolationFPS", 30);
    }

    if (CVarGetInteger("gMatchRefreshRate", 0)) {
        return LUS::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
    }

    return std::min<uint32_t>(LUS::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate(), CVarGetInteger("gInterpolationFPS", 30));
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

        s16 audio_buffer[SAMPLES_PER_FRAME];
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
    auto resourceMgr = LUS::Context::GetInstance()->GetResourceManager();
    resourceMgr->LoadDirectory("sound");
    auto banksFiles = resourceMgr->GetArchive()->ListFiles("sound/banks/*");

    for(auto& bank : *banksFiles){
        auto path = "__OTR__" + bank;
        auto ctl = static_cast<CtlEntry *>(ResourceGetDataByName(path.c_str()));
        this->bankMapTable[bank] = ctl->bankId;
    }

    auto sequencesFiles = resourceMgr->GetArchive()->ListFiles("sound/sequences/*");

    for(auto& sequence : *sequencesFiles){
        auto path = "__OTR__" + sequence;
        auto seq = static_cast<AudioSequenceData *>(ResourceGetDataByName(path.c_str()));
        this->sequencesMapTable[seq->id] = path;
    }

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

void GameEngine::LoadDictionary() {
    this->dictionary = static_cast<std::unordered_map<std::string, std::vector<uint8_t>> *>(ResourceGetDataByName("__OTR__texts/strings/global"));
}

uint8_t GameEngine::GetBankIdByName(const std::string& name) {
    auto engine = GameEngine::Instance;
    if(engine->bankMapTable.contains(name)){
        return engine->bankMapTable[name];
    }
    return 0;
}

extern "C" uint32_t GameEngine_GetInterpolatedFPS() {
    return GameEngine::GetInterpolationFPS();
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

extern "C" uint32_t GameEngine_GetSamplesPerFrame(){
    return SAMPLES_PER_FRAME;
}

// End

extern "C" float GameEngine_GetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}

extern "C" CtlEntry* GameEngine_LoadBank(uint8_t bankId) {
    auto engine = GameEngine::Instance;
    if(bankId >= engine->bankMapTable.size()){
        return nullptr;
    }
    if(engine->banks.contains(bankId)){
        return engine->banks[bankId];
    }
    for(auto& bank : engine->bankMapTable){
        if(bank.second == bankId){
            auto ctl = static_cast<CtlEntry *>(ResourceGetDataByName(("__OTR__" + bank.first).c_str()));
            engine->banks[bankId] = ctl;
            return ctl;
        }
    }
    return nullptr;
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
    if(!engine->sequencesMapTable.contains(seqId)){
        return nullptr;
    }

    if(engine->sequences.contains(seqId)){
        return engine->sequences[seqId];
    }

    auto sequences = static_cast<AudioSequenceData *>(ResourceGetDataByName(engine->sequencesMapTable[seqId].c_str()));
    engine->sequences[seqId] = sequences;
    return sequences;
}

extern "C" uint32_t GameEngine_GetSequenceCount(){
    auto engine = GameEngine::Instance;
    return engine->sequencesMapTable.size();
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

extern "C" uint8_t* GameEngine_LoadActName(uint32_t actId){
    return (uint8_t*) ResourceGetDataByName(StringHelper::Sprintf(gActRoot, actId).c_str());
}

extern "C" uint8_t* GameEngine_LoadLevelName(uint32_t courseId){
    return (uint8_t*) ResourceGetDataByName(StringHelper::Sprintf(gCourseRoot, courseId).c_str());
}

extern "C" DialogEntry* GameEngine_LoadDialog(uint32_t dialogId){
    auto dialog = (DialogEntry*) ResourceGetDataByName(StringHelper::Sprintf(gDialogRoot, dialogId).c_str());
    return dialog;
}

extern "C" uint8_t* GameEngine_LoadTranslation(const char* key) {
    auto engine = GameEngine::Instance;
    auto dictionary = engine->dictionary;

    assert(engine->dictionary != nullptr);
    assert(engine->dictionary->contains(key));

    return engine->dictionary->at(key).data();
}