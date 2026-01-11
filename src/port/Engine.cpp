#include "Engine.h"
#include "ui/GhostshipGui.hpp"
#include "GameExtractor.h"
#include "ShipInit.hpp"
#include "port/importer/AnimationFactory.h"
#include "port/importer/AudioBankFactory.h"
#include "port/importer/TrajectoryFactory.h"
#include "port/importer/MovtexFactory.h"
#include "port/importer/MovtexQuadFactory.h"
#include "port/importer/PaintingFactory.h"
#include "port/importer/AudioSampleFactory.h"
#include "port/importer/AudioSequenceFactory.h"
#include "port/importer/DialogFactory.h"
#include "port/importer/DictionaryFactory.h"
#include "port/importer/ResourceType.h"
#include "port/interpolation/FrameInterpolation.h"
#include "audio/GameAudio.h"
#include "texts_table.h"
#include "port/ui/cvar_prefixes.h"
#include "port/mods/PortEnhancements.h"
#include "port/console/DevConsole.h"
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <SDL2/SDL.h>

#ifdef USE_NETWORKING
#include <SDL2/SDL_net.h>
#endif

#include <fast/resource/ResourceType.h>
#include <ship/window/gui/Fonts.h>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/factory/TextureFactory.h>
#include <fast/resource/factory/MatrixFactory.h>
#include <fast/resource/factory/VertexFactory.h>
#include <fast/resource/factory/LightFactory.h>
#include <ship/resource/factory/BlobFactory.h>
#include <ship/utils/StringHelper.h>
#include <ship/resource/ResourceType.h>
#include <ship/window/gui/resource/Font.h>

#include "importer/AssetArrayFactory.h"
#include "port/importer/GenericArrayFactory.h"
#include "controller/controldeck/ControlDeck.h"
#include <filesystem>

#ifdef __SWITCH__
#include <port/switch/SwitchImpl.h>
#endif

const float imguiScaleOptionToValue[4] = { 0.75f, 1.0f, 1.5f, 2.0f };
const uint32_t defaultImGuiScale = 1;
int32_t previousImGuiScaleIndex = -1;
float previousImGuiScale = defaultImGuiScale;

namespace fs = std::filesystem;

extern "C" {
#include "sm64.h"
#include "audio/external.h"
#include "audio/internal.h"
#include "game/ingame_menu.h"
bool prevAltAssets = false;
}

GameEngine* GameEngine::Instance;

GameEngine::GameEngine() : dictionary(nullptr) {
    this->context = Ship::Context::CreateUninitializedInstance("Ghostship", "sm64", "ghostship.cfg.json");

#ifdef __SWITCH__
    Ship::Switch::Init(Ship::PreInitPhase);
    Ship::Switch::Init(Ship::PostInitPhase);
#endif

    std::vector<std::string> archiveFiles;
    const std::string main_path = Ship::Context::GetPathRelativeToAppDirectory("sm64.o2r");
    const std::string assets_path = Ship::Context::LocateFileAcrossAppDirs("ghostship.o2r");

#ifdef _WIN32
    AllocConsole();
#endif

    if (std::filesystem::exists(main_path)) {
        archiveFiles.push_back(main_path);
    } else {
        if (ShowYesNoBox("Ghostship - Asset Extraction",
                         "Please provide a Super Mario 64 ROM.\n\nSupported Versions:\nUS\nJP\n\nAssets will be "
                         "extracted into an O2R file.") == IDYES) {
            if (!GenAssetFile()) {
                ShowMessage("Error", "An error occured, no O2R file was generated.\n\nExiting...");
                exit(1);
            } else {
                archiveFiles.push_back(main_path);
            }
        } else {
            exit(1);
        }
    }

    if (std::filesystem::exists(assets_path)) {
        archiveFiles.push_back(assets_path);
    }

    if (const std::string patches_path = Ship::Context::GetPathRelativeToAppDirectory("mods"); !patches_path.empty()) {
        if (!std::filesystem::exists(patches_path)) {
            std::filesystem::create_directories(patches_path);
        }

        if (std::filesystem::is_directory(patches_path)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(patches_path)) {
                const auto ext = p.path().extension().string();
                if (StringHelper::IEquals(ext, ".otr") || StringHelper::IEquals(ext, ".o2r")) {
                    archiveFiles.push_back(p.path().generic_string());
                }

                if (StringHelper::IEquals(ext, ".zip")) {
                    SPDLOG_WARN("Zip files should be only used for development purposes, not for distribution");
                    archiveFiles.push_back(p.path().generic_string());
                }
            }
        }
    }

    this->context->InitConfiguration();    // without this line InitConsoleVariables fails at Config::Reload()
    this->context->InitConsoleVariables(); // without this line the controldeck constructor failes in
                                           // ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig()

    auto controlDeck = std::make_shared<LUS::ControlDeck>();

    this->context->InitResourceManager(archiveFiles, {}, 3); // without this line InitWindow fails in Gui::Init()
    this->context->InitConsole(); // without this line the GuiWindow constructor fails in ConsoleWindow::InitElement()

    auto window = std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>({}));

    this->context->Init(archiveFiles, {}, 3, { 32000, 512, 1100 }, window, controlDeck);

#ifndef __SWITCH__
    Ship::Context::GetInstance()->GetLogger()->set_level(
        (spdlog::level::level_enum)CVarGetInteger("gDeveloperTools.LogLevel", 1));
    Ship::Context::GetInstance()->GetLogger()->set_pattern("[%H:%M:%S.%e] [%s:%#] [%l] %v");
#endif

    Ship::Context::GetInstance()->GetLogger()->set_level(
        (spdlog::level::level_enum)CVarGetInteger("gDeveloperTools.LogLevel", 1));
    Ship::Context::GetInstance()->GetLogger()->set_pattern("[%H:%M:%S.%e] [%s:%#] [%l] %v");

    window->SetTargetFps(60);
    window->SetMaximumFrameLatency(1);
    window->SetRendererUCode(ucode_f3d);

    auto loader = context->GetResourceManager()->GetResourceLoader();
    auto blobFactory = std::make_shared<Ship::ResourceFactoryBinaryBlobV0>();
    loader->RegisterResourceFactory(std::make_shared<SM64::AnimationFactoryV0>(), RESOURCE_FORMAT_BINARY, "Animation",
                                    static_cast<uint32_t>(SM64::ResourceType::Anim), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioBankFactoryV0>(), RESOURCE_FORMAT_BINARY, "AudioBank",
                                    static_cast<uint32_t>(SM64::ResourceType::Bank), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioSampleFactoryV0>(), RESOURCE_FORMAT_BINARY,
                                    "AudioSample", static_cast<uint32_t>(SM64::ResourceType::Sample), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioSequenceFactoryV0>(), RESOURCE_FORMAT_BINARY,
                                    "AudioSequence", static_cast<uint32_t>(SM64::ResourceType::Sequence), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::DialogFactoryV0>(), RESOURCE_FORMAT_BINARY, "Dialog",
                                    static_cast<uint32_t>(SM64::ResourceType::SDialog), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::DictionaryFactoryV0>(), RESOURCE_FORMAT_BINARY, "Dictionary",
                                    static_cast<uint32_t>(SM64::ResourceType::Dictionary), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY,
                                    "Vertex", static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
                                    RESOURCE_FORMAT_BINARY, "DisplayList",
                                    static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY,
                                    "Matrix", static_cast<uint32_t>(Fast::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryLightV0>(), RESOURCE_FORMAT_BINARY,
                                    "Light", static_cast<uint32_t>(Fast::ResourceType::Light), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::ResourceFactoryBinaryAssetArrayV0>(), RESOURCE_FORMAT_BINARY,
                                    "AssetArray", static_cast<uint32_t>(SM64::ResourceType::AssetArray), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::TrajectoryFactoryV0>(), RESOURCE_FORMAT_BINARY, "Trajectory",
                                    static_cast<uint32_t>(SM64::ResourceType::Trajectory), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::MovtexFactoryV0>(), RESOURCE_FORMAT_BINARY, "Movtex",
                                    static_cast<uint32_t>(SM64::ResourceType::Movtex), 0);
    // TODO: This shit needs to change, i mean why i have 5 factories doing the same thing xD
    loader->RegisterResourceFactory(std::make_shared<SF64::ResourceFactoryBinaryGenericArrayV0>(),
                                    RESOURCE_FORMAT_BINARY, "GenericArray",
                                    static_cast<uint32_t>(SM64::ResourceType::GenericArray), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::MovtexFactoryV0>(), RESOURCE_FORMAT_BINARY, "Collision",
                                    static_cast<uint32_t>(SM64::ResourceType::Collision), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::MovtexFactoryV0>(), RESOURCE_FORMAT_BINARY, "PaintingData",
                                    static_cast<uint32_t>(SM64::ResourceType::PaintingData), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::MovtexFactoryV0>(), RESOURCE_FORMAT_BINARY, "MacroObject",
                                    static_cast<uint32_t>(SM64::ResourceType::MacroObject), 0);

    loader->RegisterResourceFactory(std::make_shared<SM64::MovtexQuadFactoryV0>(), RESOURCE_FORMAT_BINARY, "MovtexQuad",
                                    static_cast<uint32_t>(SM64::ResourceType::MovtexQuad), 0);
    loader->RegisterResourceFactory(std::make_shared<SM64::PaintingFactoryV0>(), RESOURCE_FORMAT_BINARY, "Painting",
                                    static_cast<uint32_t>(SM64::ResourceType::Painting), 0);

    loader->RegisterResourceFactory(blobFactory, RESOURCE_FORMAT_BINARY, "Blob",
                                    static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    prevAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0);
    context->GetResourceManager()->SetAltAssetsEnabled(prevAltAssets);

    fontMono = CreateFontWithSize(16.0f, "fonts/Inconsolata-Regular.ttf");
    fontMonoLarger = CreateFontWithSize(20.0f, "fonts/Inconsolata-Regular.ttf");
    fontMonoLargest = CreateFontWithSize(24.0f, "fonts/Inconsolata-Regular.ttf");
    fontStandard = CreateFontWithSize(16.0f, "fonts/Montserrat-Regular.ttf");
    fontStandardLarger = CreateFontWithSize(20.0f, "fonts/Montserrat-Regular.ttf");
    fontStandardLargest = CreateFontWithSize(24.0f, "fonts/Montserrat-Regular.ttf");
    ImGui::GetIO().FontDefault = fontMono;
}

bool GameEngine::GenAssetFile(bool exitOnFail) {
    auto extractor = new GameExtractor();

    if (!extractor->SelectGameFromUI()) {
        ShowMessage("Error", "No ROM selected.\n\nExiting...");
        if (exitOnFail) {
            exit(1);
        } else {
            return false;
        }
    }

    auto game = extractor->ValidateChecksum();
    if (!game.has_value()) {
        ShowMessage("Unsupported ROM",
                    "The provided ROM is not supported.\n\nCheck the readme for a list of supported versions.");
        if (exitOnFail) {
            exit(1);
        } else {
            return false;
        }
    }

    ShowMessage(("Ghostship - Extraction - Found " + game.value()).c_str(),
                "The extraction process will now begin.\n\nThis may take a few minutes.", SDL_MESSAGEBOX_INFORMATION);

    return extractor->GenerateOTR();
}

void GameEngine::ShowMessage(const char* title, const char* message, SDL_MessageBoxFlags type) {
#if defined(__SWITCH__)
    SPDLOG_ERROR(message);
#else
    SDL_ShowSimpleMessageBox(type, title, message, nullptr);
    SPDLOG_ERROR(message);
#endif
}

int GameEngine::ShowYesNoBox(const char* title, const char* box) {
    int ret;
#ifdef _WIN32
    ret = MessageBoxA(nullptr, box, title, MB_YESNO | MB_ICONQUESTION);
#elif defined(__SWITCH__)
    SPDLOG_ERROR(box);
    return IDYES;
#else
    SDL_MessageBoxData boxData = { 0 };
    SDL_MessageBoxButtonData buttons[2] = { { 0 } };

    buttons[0].buttonid = IDYES;
    buttons[0].text = "Yes";
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    buttons[1].buttonid = IDNO;
    buttons[1].text = "No";
    buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    boxData.numbuttons = 2;
    boxData.flags = SDL_MESSAGEBOX_INFORMATION;
    boxData.message = box;
    boxData.title = title;
    boxData.buttons = buttons;
    SDL_ShowMessageBox(&boxData, &ret);
#endif
    return ret;
}

ImFont* GameEngine::CreateFontWithSize(float size, std::string fontPath) {
    auto mImGuiIo = &ImGui::GetIO();
    ImFont* font;
    if (fontPath == "") {
        ImFontConfig fontCfg = ImFontConfig();
        fontCfg.OversampleH = fontCfg.OversampleV = 1;
        fontCfg.PixelSnapH = true;
        fontCfg.SizePixels = size;
        font = mImGuiIo->Fonts->AddFontDefault(&fontCfg);
    } else {
        auto initData = std::make_shared<Ship::ResourceInitData>();
        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;

        initData->Format = RESOURCE_FORMAT_BINARY;
        initData->Type = static_cast<uint32_t>(RESOURCE_TYPE_FONT);
        initData->ResourceVersion = 0;
        initData->Path = fontPath;
        std::shared_ptr<Ship::Font> fontData = std::static_pointer_cast<Ship::Font>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResource(fontPath, false, initData));
        font = mImGuiIo->Fonts->AddFontFromMemoryTTF(fontData->Data, fontData->DataSize, size, &config);
    }
    // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    float iconFontSize = size * 2.0f / 3.0f;
    static const ImWchar sIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    mImGuiIo->Fonts->AddFontFromMemoryCompressedBase85TTF(fontawesome_compressed_data_base85, iconFontSize,
                                                          &iconsConfig, sIconsRanges);

    return font;
}

void GameEngine::ScaleImGui() {
    int32_t imGuiScaleIndex = CVarGetInteger("gSettings.ImGuiScale", defaultImGuiScale);
    if (imGuiScaleIndex == previousImGuiScaleIndex) {
        return;
    }

    float scale = imguiScaleOptionToValue[imGuiScaleIndex];
    float newScale = scale / previousImGuiScale;
    ImGui::GetStyle().ScaleAllSizes(newScale);
    ImGui::GetIO().FontGlobalScale = scale;
    previousImGuiScale = scale;
    previousImGuiScaleIndex = imGuiScaleIndex;
}

void GameEngine::Create() {
    const auto instance = Instance = new GameEngine();
    GhostshipGui::SetupGuiElements();
    ShipInit::InitAll();
    instance->AudioInit();
    instance->LoadDictionary();
    instance->LoadPlayerAnims();
#if defined(__SWITCH__) || defined(__WIIU__)
    CVarRegisterInteger("gControlNav", 1); // always enable controller nav on switch/wii u
#endif
    DevConsole_Init();
}

void GameEngine::Destroy() {
    GhostshipGui::Destroy();
    Instance->context = nullptr;
    AudioExit();
#ifdef __SWITCH__
    Ship::Switch::Exit();
#endif
}

void GameEngine::StartFrame() const {
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
        case KbScancode::LUS_KB_TAB: {
            // Toggle HD Assets
            CVarSetInteger("gEnhancements.Mods.AlternateAssets",
                           !CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0));
            break;
        }
        default:
            break;
    }
}

uint32_t GameEngine::GetInterpolationFPS() {
    if (Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() == Ship::WindowBackend::FAST3D_DXGI_DX11) {
        return CVarGetInteger(CVAR_SETTING("InterpolationFPS"), 30);
    }

    if (CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0)) {
        return Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
    }

    return std::min<uint32_t>(Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate(),
                              CVarGetInteger(CVAR_SETTING("InterpolationFPS"), 30));
}

// Audio

void GameEngine::HandleAudioThread() {
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

        float master_vol = CVarGetInteger("gSettings.Volume.Master", 100) / 100.0f;

        for (u32 i = 0; i < SAMPLES_PER_FRAME; i++) {
            audio_buffer[i] = static_cast<s16>(audio_buffer[i] * master_vol);
        }

        AudioPlayerPlayFrame((u8*)audio_buffer, 2 * num_audio_samples * 4);

        audio.processing = false;
        audio.cv_from_thread.notify_one();
    }
}

void GameEngine::StartAudioFrame() {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.processing = true;
    }

    audio.cv_to_thread.notify_one();
}

void GameEngine::EndAudioFrame() {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        while (audio.processing) {
            audio.cv_from_thread.wait(Lock);
        }
    }
}

void GameEngine::AudioInit() {
    const auto resourceMgr = Ship::Context::GetInstance()->GetResourceManager();
    resourceMgr->LoadResources("sound");
    const auto banksFiles = resourceMgr->GetArchiveManager()->ListFiles("sound/banks/*");
    const auto sequences_files = resourceMgr->GetArchiveManager()->ListFiles("sound/sequences/*");

    Instance->sequenceTable.resize(512);
    Instance->audioSequenceTable.resize(512);
    Instance->banksTable.resize(512);

    for (auto& bank : *banksFiles) {
        auto path = "__OTR__" + bank;
        const auto ctl = static_cast<CtlEntry*>(ResourceGetDataByName(path.c_str()));
        this->bankMapTable[bank] = ctl->bankId;
    }

    for (auto& sequence : *sequences_files) {
        auto path = "__OTR__" + sequence;
        auto seq = static_cast<AudioSequenceData*>(ResourceGetDataByName(path.c_str()));
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
    this->dictionary = static_cast<std::unordered_map<std::string, std::vector<uint8_t>>*>(
        ResourceGetDataByName("__OTR__texts/strings/global"));
}

void GameEngine::LoadPlayerAnims() {
    auto resourceMgr = Ship::Context::GetInstance()->GetResourceManager();
    auto archiveMgr = resourceMgr->GetArchiveManager();
    auto anims = archiveMgr->ListFiles("assets/anims/*");
    this->animationsTable.resize(anims->size());

    for (auto& anim : *anims) {
        const auto id = std::stoi(anim.substr(anim.find('_') + 1, anim.length()), nullptr, 16);
        this->animationsTable[id] = static_cast<Animation*>(ResourceGetDataByName(anim.c_str()));
    }
}

uint8_t GameEngine::GetBankIdByName(const std::string& name) {
    if (Instance->bankMapTable.contains(name)) {
        return Instance->bankMapTable[name];
    }
    return 0;
}

uint32_t GameEngine::GetGameVersion() {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[0];
}

void GameEngine::RunCommands(Gfx* Commands, const std::vector<std::unordered_map<Mtx*, MtxF>>& mtx_replacements) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    if (wnd == nullptr) {
        return;
    }

    auto interpreter = wnd->GetInterpreterWeak().lock().get();

    // Process window events for resize, mouse, keyboard events
    wnd->HandleEvents();

    interpreter->mInterpolationIndex = 0;
    for (const auto& mtxStack : mtx_replacements) {
        wnd->DrawAndRunGraphicsCommands(Commands, mtxStack);
        interpreter->mInterpolationIndex++;
    }

    bool curAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0);
    if (prevAltAssets != curAltAssets) {
        prevAltAssets = curAltAssets;
        Ship::Context::GetInstance()->GetResourceManager()->SetAltAssetsEnabled(curAltAssets);
        gfx_texture_cache_clear();
    }
}

void GameEngine::ProcessGfxCommands(Gfx* commands) {
    std::vector<std::unordered_map<Mtx*, MtxF>> mtx_replacements;
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    int target_fps = GetInterpolationFPS();
    static int last_fps;
    static int time;
    int fps = target_fps;
    int original_fps = 60 / 2;

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
            mtx_replacements.push_back(FrameInterpolation_Interpolate((float)time / next_original_frame));
        } else {
            mtx_replacements.emplace_back(); // No interpolation for key frames
        }
    }

    time -= fps;

    if (wnd != nullptr) {
        wnd->SetTargetFps(GetInterpolationFPS());
        wnd->SetMaximumFrameLatency(1);
    }
    RunCommands(commands, mtx_replacements);

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

extern "C" uint32_t GameEngine_GetSamplesPerFrame() {
    return SAMPLES_PER_FRAME;
}

// End

Fast::Interpreter* GameEngine_GetInterpreter() {
    return static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
        ->GetInterpreterWeak()
        .lock()
        .get();
}

extern "C" float GameEngine_GetAspectRatio() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.aspect_ratio;
}

extern "C" CtlEntry* GameEngine_LoadBank(const uint8_t bankId) {
    const auto engine = GameEngine::Instance;

    if (bankId >= engine->bankMapTable.size()) {
        return nullptr;
    }

    if (engine->banksTable[bankId] != nullptr) {
        return engine->banksTable[bankId];
    }

    for (auto& bank : engine->bankMapTable) {
        if (bank.second == bankId) {
            const auto ctl = static_cast<CtlEntry*>(ResourceGetDataByName(("__OTR__" + bank.first).c_str()));
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

    if (engine->sequenceTable[seqId].empty()) {
        return nullptr;
    }

    if (engine->audioSequenceTable[seqId] != nullptr) {
        return engine->audioSequenceTable[seqId];
    }

    auto sequences = static_cast<AudioSequenceData*>(ResourceGetDataByName(engine->sequenceTable[seqId].c_str()));
    engine->audioSequenceTable[seqId] = sequences;
    return sequences;
}

extern "C" uint32_t GameEngine_GetSequenceCount() {
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

extern "C" uint8_t* GameEngine_LoadActName(const uint32_t actId) {
    return static_cast<uint8_t*>(ResourceGetDataByName(StringHelper::Sprintf(gActRoot, actId).c_str()));
}

extern "C" uint8_t* GameEngine_LoadLevelName(const uint32_t courseId) {
    return static_cast<uint8_t*>(ResourceGetDataByName(StringHelper::Sprintf(gCourseRoot, courseId).c_str()));
}

extern "C" DialogEntry* GameEngine_LoadDialog(const uint32_t dialogId) {
    auto dialog =
        static_cast<DialogEntry*>(ResourceGetDataByName(StringHelper::Sprintf(gDialogRoot, dialogId).c_str()));
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
    if (animId >= engine->animationsTable.size()) {
        return nullptr;
    }
    return engine->animationsTable[animId];
}

// Gets the width of the main ImGui window
extern "C" uint32_t OTRGetCurrentWidth() {
    return GameEngine::Instance->context->GetWindow()->GetWidth();
}

// Gets the height of the main ImGui window
extern "C" uint32_t OTRGetCurrentHeight() {
    return GameEngine::Instance->context->GetWindow()->GetHeight();
}

extern "C" float OTRGetHUDAspectRatio() {
    if (CVarGetInteger("gHUDAspectRatio.Enabled", 0) == 0 || CVarGetInteger("gHUDAspectRatio.X", 0) == 0 ||
        CVarGetInteger("gHUDAspectRatio.Y", 0) == 0) {
        return GameEngine_GetAspectRatio();
    }
    return ((float)CVarGetInteger("gHUDAspectRatio.X", 1) / (float)CVarGetInteger("gHUDAspectRatio.Y", 1));
}

extern "C" float OTRGetDimensionFromLeftEdge(float v) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 -
            interpreter->mNativeDimensions.height / 2 * interpreter->mCurDimensions.aspect_ratio + (v));
}

extern "C" float OTRGetDimensionFromRightEdge(float v) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 +
            interpreter->mNativeDimensions.height / 2 * interpreter->mCurDimensions.aspect_ratio - (v));
}

extern "C" float OTRGetDimensionFromLeftEdgeForcedAspect(float v, float aspectRatio) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 -
            interpreter->mNativeDimensions.height / 2 *
                (aspectRatio > 0 ? aspectRatio : interpreter->mCurDimensions.aspect_ratio) +
            (v));
}

extern "C" float OTRGetDimensionFromRightEdgeForcedAspect(float v, float aspectRatio) {
    auto interpreter = GameEngine_GetInterpreter();
    return (interpreter->mNativeDimensions.width / 2 +
            interpreter->mNativeDimensions.height / 2 *
                (aspectRatio > 0 ? aspectRatio : interpreter->mCurDimensions.aspect_ratio) -
            (v));
}

extern "C" float OTRGetDimensionFromLeftEdgeOverride(float v) {
    return OTRGetDimensionFromLeftEdgeForcedAspect(v, OTRGetHUDAspectRatio());
}

extern "C" float OTRGetDimensionFromRightEdgeOverride(float v) {
    return OTRGetDimensionFromRightEdgeForcedAspect(v, OTRGetHUDAspectRatio());
}

// Gets the width of the current render target area
extern "C" uint32_t OTRGetGameRenderWidth() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.width;
}

// Gets the height of the current render target area
extern "C" uint32_t OTRGetGameRenderHeight() {
    auto interpreter = GameEngine_GetInterpreter();
    return interpreter->mCurDimensions.height;
}

extern "C" int16_t OTRGetRectDimensionFromLeftEdge(float v) {
    return ((int)floorf(OTRGetDimensionFromLeftEdge(v)));
}

extern "C" int16_t OTRGetRectDimensionFromRightEdge(float v) {
    return ((int)ceilf(OTRGetDimensionFromRightEdge(v)));
}

extern "C" int16_t OTRGetRectDimensionFromLeftEdgeForcedAspect(float v, float aspectRatio) {
    return ((int)floorf(OTRGetDimensionFromLeftEdgeForcedAspect(v, aspectRatio)));
}

extern "C" int16_t OTRGetRectDimensionFromRightEdgeForcedAspect(float v, float aspectRatio) {
    return ((int)ceilf(OTRGetDimensionFromRightEdgeForcedAspect(v, aspectRatio)));
}

extern "C" int16_t OTRGetRectDimensionFromLeftEdgeOverride(float v) {
    return OTRGetRectDimensionFromLeftEdgeForcedAspect(v, OTRGetHUDAspectRatio());
}

extern "C" int16_t OTRGetRectDimensionFromRightEdgeOverride(float v) {
    return OTRGetRectDimensionFromRightEdgeForcedAspect(v, OTRGetHUDAspectRatio());
}

extern "C" int32_t OTRConvertHUDXToScreenX(int32_t v) {
    auto interpreter = GameEngine_GetInterpreter();
    float gameAspectRatio = interpreter->mCurDimensions.aspect_ratio;
    int32_t gameHeight = interpreter->mCurDimensions.height;
    int32_t gameWidth = interpreter->mCurDimensions.width;
    float hudAspectRatio = 4.0f / 3.0f;
    int32_t hudHeight = gameHeight;
    int32_t hudWidth = hudHeight * hudAspectRatio;
    float hudScreenRatio = (hudWidth / 320.0f);
    float hudCoord = v * hudScreenRatio;
    float gameOffset = (gameWidth - hudWidth) / 2;
    float gameCoord = hudCoord + gameOffset;
    float gameScreenRatio = (320.0f / gameWidth);
    float screenScaledCoord = gameCoord * gameScreenRatio;
    int32_t screenScaledCoordInt = screenScaledCoord;
    return screenScaledCoordInt;
}