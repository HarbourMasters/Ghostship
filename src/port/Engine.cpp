#include "Engine.h"
#include "ui/ImguiUI.h"
#include "port/importer/AnimationFactory.h"
#include "port/importer/AudioBankFactory.h"
#include "port/importer/AudioSampleFactory.h"
#include "port/importer/AudioSequenceFactory.h"
#include "port/importer/DialogFactory.h"
#include "port/importer/DictionaryFactory.h"
#include "port/importer/ResourceType.h"
#include "audio/GameAudio.h"
#include "texts_table.h"
#include "port/Enhancements/game-interactor/GameInteractor.h"
#include "port/Enhancements/mods.h"
#include <Fast3D/gfx_pc.h>
#include <Fast3D/gfx_rendering_api.h>

#ifdef USE_NETWORKING
#include <SDL2/SDL_net.h>
#endif

#include <utility>

#include "ArrayFactory.h"
#include "BlobFactory.h"
#include "DisplayListFactory.h"
#include "MatrixFactory.h"
#include "StringHelper.h"
#include "TextureFactory.h"
#include "VertexFactory.h"
#include "Fast3D/Fast3dWindow.h"

extern "C" {
#include "sm64.h"
#include "audio/external.h"
#include "audio/internal.h"
#include "game/ingame_menu.h"
float gInterpolationStep = 0.0f;
}

GameEngine* GameEngine::Instance;
GameInteractor* GameInteractor::Instance;

GameEngine::GameEngine(): dictionary(nullptr) {
    std::vector<std::string> OTRFiles;
    if (const std::string cube_path = Ship::Context::GetPathRelativeToAppDirectory("smcube.otr"); std::filesystem::exists(cube_path)) {
        OTRFiles.push_back(cube_path);
    }
    if (const std::string sm64_otr_path = Ship::Context::GetPathRelativeToAppBundle("sm64.otr"); std::filesystem::exists(sm64_otr_path)) {
        OTRFiles.push_back(sm64_otr_path);
    }
    if (const std::string patches_path = Ship::Context::GetPathRelativeToAppDirectory("mods"); !patches_path.empty() && std::filesystem::exists(patches_path)) {
        if (std::filesystem::is_directory(patches_path)) {
            for (const auto&p: std::filesystem::recursive_directory_iterator(patches_path)) {
                if (StringHelper::IEquals(p.path().extension().string(), ".otr")) {
                    OTRFiles.push_back(p.path().generic_string());
                }
            }
        }
    }
    this->context = Ship::Context::CreateInstance("Ghostship", "sm64", "ghostship.cfg.json", OTRFiles,
                                                 {0xFF2B5A63, 0xE3DAA4E}, 3);

    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    wnd->SetTargetFps(60);
    wnd->SetMaximumFrameLatency(1);
    wnd->SetRendererUCode(ucode_f3d);

    auto loader = context->GetResourceManager()->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<SM64::AnimationFactoryV0>(), RESOURCE_FORMAT_BINARY, "Animation", static_cast<uint32_t>(SM64::ResourceType::Anim), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioBankFactoryV0>(), RESOURCE_FORMAT_BINARY, "AudioBank", static_cast<uint32_t>(SM64::ResourceType::Bank), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioSampleFactoryV0>(), RESOURCE_FORMAT_BINARY, "AudioSample", static_cast<uint32_t>(SM64::ResourceType::Sample), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioSequenceFactoryV0>(), RESOURCE_FORMAT_BINARY, "AudioSequence", static_cast<uint32_t>(SM64::ResourceType::Sequence), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::DialogFactoryV0>(), RESOURCE_FORMAT_BINARY, "Dialog", static_cast<uint32_t>(SM64::ResourceType::SDialog), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::DictionaryFactoryV0>(), RESOURCE_FORMAT_BINARY, "Dictionary", static_cast<uint32_t>(SM64::ResourceType::Dictionary), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(LUS::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(LUS::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY, "Vertex", static_cast<uint32_t>(LUS::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryDisplayListV0>(), RESOURCE_FORMAT_BINARY, "DisplayList", static_cast<uint32_t>(LUS::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY, "Matrix", static_cast<uint32_t>(LUS::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryArrayV0>(), RESOURCE_FORMAT_BINARY, "Array", static_cast<uint32_t>(LUS::ResourceType::Array), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY, "Blob", static_cast<uint32_t>(LUS::ResourceType::Blob), 0);
}

void GameEngine::Create(){
    const auto instance = Instance = new GameEngine();
    GameInteractor::Instance = new GameInteractor();
    InitMods();
    GameUI::SetupGuiElements();
    instance->AudioInit();
    instance->LoadDictionary();
    instance->LoadPlayerAnims();
}

void GameEngine::Destroy(){
    AudioExit();
}

bool ShouldClearTextureCacheAtEndOfFrame = false;

void GameEngine::StartFrame() const{
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
        case KbScancode::LUS_KB_TAB: {
            // Toggle HD Assets
            CVarSetInteger("gAltAssets", !CVarGetInteger("gAltAssets", 0));
            ShouldClearTextureCacheAtEndOfFrame = true;
            break;
        }
        default: break;
    }
    this->context->GetWindow()->StartFrame();
}

uint32_t GameEngine::GetInterpolationFPS() {
    if (Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() == Ship::WindowBackend::DX11) {
        return CVarGetInteger("gInterpolationFPS", 30);
    }

    if (CVarGetInteger("gMatchRefreshRate", 0)) {
        return Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
    }

    return std::min<uint32_t>(Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate(), CVarGetInteger("gInterpolationFPS", 30));
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
        for (int i = 0; i < NUM_AUDIO_CHANNELS; i++) {
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
    const auto resourceMgr = Ship::Context::GetInstance()->GetResourceManager();
    resourceMgr->LoadDirectory("sound");
    const auto banksFiles = resourceMgr->GetArchiveManager()->ListFiles("sound/banks/*");
    const auto sequences_files = resourceMgr->GetArchiveManager()->ListFiles("sound/sequences/*");

    Instance->sequenceTable.resize(512);
    Instance->audioSequenceTable.resize(512);
    Instance->banksTable.resize(512);

    for(auto& bank : *banksFiles){
        auto path = "__OTR__" + bank;
        const auto ctl = static_cast<CtlEntry *>(ResourceGetDataByName(path.c_str()));
        this->bankMapTable[bank] = ctl->bankId;
    }

    for( auto& sequence : *sequences_files){
        auto path = "__OTR__" + sequence;
        auto seq = static_cast<AudioSequenceData *>(ResourceGetDataByName(path.c_str()));
        Instance->sequenceTable[seq->id] = path;
    }

    if (!audio.running) {
        audio.running = true;
        audio.thread = std::thread(HandleAudioThread);
    }
}

void GameEngine::AudioExit() {
    {
        std::unique_lock lock(audio.mutex);
        audio.running = false;
    }
    audio.cv_to_thread.notify_all();

    // Wait until the audio thread quit
    audio.thread.join();
}

void GameEngine::LoadDictionary() {
    this->dictionary = static_cast<std::unordered_map<std::string, std::vector<uint8_t>> *>(ResourceGetDataByName("__OTR__texts/strings/global"));
}

void GameEngine::LoadPlayerAnims() {
    auto resourceMgr = Ship::Context::GetInstance()->GetResourceManager();
    auto archiveMgr = resourceMgr->GetArchiveManager();
    auto anims = archiveMgr->ListFiles("assets/anims/*");
    this->animationsTable.resize(anims->size());

    for(auto& anim : *anims){
        const auto id = std::stoi(anim.substr(anim.find('_') + 1, anim.length()), nullptr, 16);
        this->animationsTable[id] = static_cast<Animation *>(ResourceGetDataByName(anim.c_str()));
    }
}

uint8_t GameEngine::GetBankIdByName(const std::string& name) {
    if(Instance->bankMapTable.contains(name)){
        return Instance->bankMapTable[name];
    }
    return 0;
}

uint32_t GameEngine::GetGameVersion() {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[0];
}

void GameEngine::RunCommands(F3DGfx* Commands) {
    gfx_run(reinterpret_cast<Gfx*>(Commands), {});
    gfx_end_frame();

    if (ShouldClearTextureCacheAtEndOfFrame) {
        gfx_texture_cache_clear();
        ShouldClearTextureCacheAtEndOfFrame = false;
    }
}

void GameEngine::PatchInterpolations() {
    mtx_patch_interpolated();
    patch_screen_transition_interpolated();
    patch_title_screen_scales();
    patch_interpolated_dialog();
    patch_interpolated_hud();
    patch_interpolated_paintings();
    patch_interpolated_bubble_particles();
    patch_interpolated_snow_particles();
}

void GameEngine::ProcessGfxCommands(F3DGfx* commands) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    int target_fps = GetInterpolationFPS();
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
            gInterpolationStep = static_cast<float>(time) / next_original_frame;
        }
        RunCommands(commands);
        PatchInterpolations();
    }

    time -= fps;

    wnd->SetTargetFps(fps);

    int threshold = CVarGetInteger("gExtraLatencyThreshold", 80);
    wnd->SetMaximumFrameLatency(threshold > 0 && target_fps >= threshold ? 2 : 1);

    last_fps = fps;
}

extern "C" uint32_t GameEngine_GetInterpolatedFPS() {
    return GameEngine::GetInterpolationFPS();
}

extern "C" uint32_t GameEngine_GetSampleRate() {
    auto player = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();
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

extern "C" CtlEntry* GameEngine_LoadBank(const uint8_t bankId) {
    const auto engine = GameEngine::Instance;

    if(bankId >= engine->bankMapTable.size()){
        return nullptr;
    }

    if(engine->banksTable[bankId] != nullptr){
        return engine->banksTable[bankId];
    }

    for(auto& bank : engine->bankMapTable){
        if(bank.second == bankId){
            const auto ctl = static_cast<CtlEntry *>(ResourceGetDataByName(("__OTR__" + bank.first).c_str()));
            engine->banksTable[bankId] = ctl;
            return ctl;
        }
    }
    return nullptr;
}

extern "C" uint8_t GameEngine_IsBankLoaded(const uint8_t bankId) {
    const auto engine = GameEngine::Instance;
    GameEngine_LoadBank(bankId);
    return engine->banksTable[bankId] != nullptr;
}

extern "C" void GameEngine_UnloadBank(const uint8_t bankId) {
    const auto engine = GameEngine::Instance;
    engine->banksTable[bankId] = nullptr;
}

extern "C" AudioSequenceData* GameEngine_LoadSequence(const uint8_t seqId) {
    auto engine = GameEngine::Instance;

    if(engine->sequenceTable[seqId].empty()){
        return nullptr;
    }

    if(engine->audioSequenceTable[seqId] != nullptr){
        return engine->audioSequenceTable[seqId];
    }

    auto sequences = static_cast<AudioSequenceData *>(ResourceGetDataByName(engine->sequenceTable[seqId].c_str()));
    engine->audioSequenceTable[seqId] = sequences;
    return sequences;
}

extern "C" uint32_t GameEngine_GetSequenceCount(){
    auto engine = GameEngine::Instance;
    return engine->sequenceTable.size();
}

extern "C" uint8_t GameEngine_IsSequenceLoaded(const uint8_t seqId) {
    return GameEngine_LoadSequence(seqId) != nullptr;
}

extern "C" void GameEngine_UnloadSequence(const uint8_t seqId) {
    const auto engine = GameEngine::Instance;
    engine->audioSequenceTable[seqId] = nullptr;
}

extern "C" uint32_t GameEngine_GetGameVersion() {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[0];
}

extern "C" uint8_t* GameEngine_LoadActName(const uint32_t actId){
    return static_cast<uint8_t *>(ResourceGetDataByName(StringHelper::Sprintf(gActRoot, actId).c_str()));
}

extern "C" uint8_t* GameEngine_LoadLevelName(const uint32_t courseId){
    return static_cast<uint8_t *>(ResourceGetDataByName(StringHelper::Sprintf(gCourseRoot, courseId).c_str()));
}

extern "C" DialogEntry* GameEngine_LoadDialog(const uint32_t dialogId){
    auto dialog = static_cast<DialogEntry *>(ResourceGetDataByName(StringHelper::Sprintf(gDialogRoot, dialogId).c_str()));
    return dialog;
}

extern "C" uint8_t* GameEngine_LoadTranslation(const char* key) {
    const auto engine = GameEngine::Instance;
    const auto dictionary = engine->dictionary;

    assert(dictionary != nullptr);
    assert(dictionary->contains(key));

    return dictionary->at(key).data();
}

extern "C" int GameEngine_OTRSigCheck(const char* data) {
    return Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(data);
}

extern "C" Animation* GameEngine_LoadAnimation(const uint32_t animId) {
    auto engine = GameEngine::Instance;
    if(animId >= engine->animationsTable.size()){
        return nullptr;
    }
    return engine->animationsTable[animId];
}