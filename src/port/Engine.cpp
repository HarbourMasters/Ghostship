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
#include <filesystem>
#include <fstream>

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
#include "importer/RawTextureFactory.h"
#include "port/importer/GenericArrayFactory.h"
#include "controller/controldeck/ControlDeck.h"
#include "port/mods/utils/GfxPrint.h"

#ifdef __SWITCH__
#include <ship/port/switch/SwitchImpl.h>
#endif

const float imguiScaleOptionToValue[4] = { 0.75f, 1.0f, 1.5f, 2.0f };
std::shared_ptr<Fast::Fast3dWindow> gsFast3dWindow;
const uint32_t defaultImGuiScale = 1;
int32_t previousImGuiScaleIndex = -1;
float previousImGuiScale = defaultImGuiScale;
bool portArchiveVersionMatch = false;
std::string assets_path;

namespace fs = std::filesystem;

extern "C" {
#include "sm64.h"
#include "audio/external.h"
#include "audio/internal.h"
#include "game/ingame_menu.h"
#include "variables.h"
bool prevAltAssets = false;
}

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} OTRVersion;

GameEngine* GameEngine::Instance;

// Read the port version from an OTR file
OTRVersion ReadPortVersionFromOTR(std::string otrPath) {
    OTRVersion version = {};

    // Use a temporary archive instance to load the otr and read the version file
    auto archive = std::make_shared<Ship::O2rArchive>(otrPath);
    if (archive->Open()) {
        auto t = archive->LoadFile("portVersion");
        if (t != nullptr && t->IsLoaded) {
            auto stream = std::make_shared<Ship::MemoryStream>(t->Buffer->data(), t->Buffer->size());
            auto reader = std::make_shared<Ship::BinaryReader>(stream);
            reader->SetEndianness(Ship::Endianness::Big);
            version.major = reader->ReadUInt16();
            version.minor = reader->ReadUInt16();
            version.patch = reader->ReadUInt16();
        } else {
            SPDLOG_WARN("Failed to read portVersion file from O2R: {}", otrPath);
        }
    } else {
        SPDLOG_WARN("Failed to open O2R for version reading: {}", otrPath);
    }

    return version;
}

// Checks the program version stored in the otr and compares the major value to soh
// For Windows/Mac/Linux if the version doesn't match, offer to
OTRVersion DetectOTRVersion(std::string fileName) {
    bool isOtrOld = false;
    std::string otrPath = Ship::Context::LocateFileAcrossAppDirs(fileName);

    // Doesn't exist so nothing to do here
    if (!std::filesystem::exists(otrPath)) {
        SPDLOG_WARN("O2R file not found at path: {}", otrPath);
        return { INT16_MAX, INT16_MAX, INT16_MAX };
    }

    return ReadPortVersionFromOTR(otrPath);
}

bool VerifyArchiveVersion(OTRVersion version) {
    return version.major == gBuildVersionMajor && version.minor == gBuildVersionMinor;
}

GameEngine::GameEngine() : dictionary(nullptr) {
    this->context = Ship::Context::CreateUninitializedInstance("Ghostship", "sm64", "ghostship.cfg.json");

#ifdef __SWITCH__
    Ship::Switch::Init(Ship::PreInitPhase);
    Ship::Switch::Init(Ship::PostInitPhase);
#endif

    this->context->InitConfiguration();    // without this line InitConsoleVariables fails at Config::Reload()
    this->context->InitConsoleVariables(); // without this line the controldeck constructor failes in
    // ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig()

    assets_path = Ship::Context::LocateFileAcrossAppDirs("ghostship.o2r");
    portArchiveVersionMatch = std::filesystem::exists(assets_path);

    auto controlDeck = std::make_shared<LUS::ControlDeck>();

    this->context->InitControlDeck(controlDeck);
    this->context->InitResourceManager({ assets_path }, {}, 3);
    this->context->InitConsole();

    gsFast3dWindow = std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>({}));
    this->context->InitWindow(gsFast3dWindow);

    GhostshipGui::SetupMenu();

    if (portArchiveVersionMatch) {
        fontMono = CreateFontWithSize(16.0f, "fonts/Inconsolata-Regular.ttf");
        fontMonoLarger = CreateFontWithSize(20.0f, "fonts/Inconsolata-Regular.ttf");
        fontMonoLargest = CreateFontWithSize(24.0f, "fonts/Inconsolata-Regular.ttf");
        fontStandard = CreateFontWithSize(16.0f, "fonts/Montserrat-Regular.ttf");
        fontStandardLarger = CreateFontWithSize(20.0f, "fonts/Montserrat-Regular.ttf");
        fontStandardLargest = CreateFontWithSize(24.0f, "fonts/Montserrat-Regular.ttf");
        ImGui::GetIO().FontDefault = fontStandardLarger;
    }

    previousImGuiScaleIndex = -1;
    previousImGuiScale = defaultImGuiScale;
    ScaleImGui();
}

typedef enum ExtractSteps {
    ES_PORT_ARCHIVE,
    ES_WINDOWS,
    ES_EXTRACT_ARGS,
    ES_EXTRACT,
    ES_VERIFY,
} ExtractSteps;

typedef enum PromptSteps {
    PS_FILE_CHECK,
    PS_LOCAL,
    PS_FIRST,
    PS_DUPE,
    PS_WAIT,
    PS_NONE,
} PromptSteps;

typedef enum WindowsSteps {
    WS_TEMP,
    WS_PERMS,
    WS_ONEDRIVE,
    WS_DONE,
} WindowsSteps;

bool IsSubpath(const std::filesystem::path& path, const std::filesystem::path& base) {
    auto rel = std::filesystem::relative(path, base);
    return !rel.empty() && rel.native()[0] != '.';
}

bool PathTestCleanup(FILE* tfile) {
    try {
        if (std::filesystem::exists("./text.txt"))
            std::filesystem::remove("./text.txt");
        if (std::filesystem::exists("./test/"))
            std::filesystem::remove("./test/");
    } catch (std::filesystem::filesystem_error const& ex) { return false; }
    return true;
}

void CheckAndCreateModFolder() {
    try {
        std::string modsPath = Ship::Context::LocateFileAcrossAppDirs("mods", "sm64");
        if (!std::filesystem::exists(modsPath)) {
            // Create mods folder relative to app dir
            modsPath = Ship::Context::GetPathRelativeToAppDirectory("mods", "sm64");
            std::string filePath = modsPath + "/custom_mod_files_go_here.txt";
            if (std::filesystem::create_directories(modsPath)) {
                std::ofstream(filePath).close();
            }
        }
    } catch (std::filesystem::filesystem_error const& ex) {
        // Couldn't make the folder, continue silently
        return;
    }
}

void GameEngine::FinishInit() {
    std::string romPath = Ship::Context::LocateFileAcrossAppDirs("sm64.o2r", "sm64");
    if (std::filesystem::exists(romPath)) {
        context->GetResourceManager()->GetArchiveManager()->AddArchive(romPath);
    }

    const std::string patches_path = Ship::Context::GetPathRelativeToAppDirectory("mods");

    if (!patches_path.empty()) {
        if (!std::filesystem::exists(patches_path)) {
            std::filesystem::create_directories(patches_path);
        }

        if (std::filesystem::is_directory(patches_path)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(patches_path)) {
                const auto ext = p.path().extension().string();
                if (StringHelper::IEquals(ext, ".otr") || StringHelper::IEquals(ext, ".o2r")) {
                    Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->AddArchive(
                        p.path().generic_string());
                }

                if (StringHelper::IEquals(ext, ".zip")) {
                    SPDLOG_WARN("Zip files should be only used for development purposes, not for distribution");
                    Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->AddArchive(
                        p.path().generic_string());
                }
            }

            for (const auto& p : std::filesystem::directory_iterator(patches_path)) {
                if (p.is_directory()) {
                    SPDLOG_INFO("Found mod directory: {}", p.path().generic_string());
                    Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->AddArchive(
                        p.path().generic_string());
                }
            }
        }
    }

#if (_DEBUG)
    auto defaultLogLevel = spdlog::level::debug;
#else
    auto defaultLogLevel = spdlog::level::info;
#endif
    auto logLevel =
        static_cast<spdlog::level::level_enum>(CVarGetInteger(CVAR_DEVELOPER_TOOLS("LogLevel"), defaultLogLevel));
    context->InitLogging(logLevel, logLevel);
    Ship::Context::GetInstance()->GetLogger()->set_pattern("[%H:%M:%S.%e] [%s:%#] [%l] %v");
    SPDLOG_INFO("Starting Ghostship version {} (Branch: {} | Commit: {})", (char*)gBuildVersion, (char*)gGitBranch,
                (char*)gGitCommitHash);

    context->InitGfxDebugger();
    context->InitFileDropMgr();
    context->InitCrashHandler();

    this->context->InitAudio({ .SampleRate = 32000, .SampleLength = 512, .DesiredBuffered = 1100 });

    gsFast3dWindow->SetTargetFps(60);
    gsFast3dWindow->SetMaximumFrameLatency(1);
    gsFast3dWindow->SetRendererUCode(ucode_f3d);

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
    loader->RegisterResourceFactory(std::make_shared<SM64::AudioSequenceXMLFactoryV0>(), RESOURCE_FORMAT_XML,
                                    "Sequence", static_cast<uint32_t>(SM64::ResourceType::Sequence), 0);
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
    prevAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1);
    context->GetResourceManager()->SetAltAssetsEnabled(prevAltAssets);

    GhostshipGui::SetupGuiElements();
    Instance->AudioInit();
    Instance->LoadDictionary();
    Instance->LoadPlayerAnims();
#if defined(__SWITCH__) || defined(__WIIU__)
    CVarRegisterInteger("gControlNav", 1); // always enable controller nav on switch/wii u
#endif
    DevConsole_Init();
    PortEnhancements_Init();
    ShipInit::InitAll();
}

void GameEngine::RunExtract(int argc, char* argv[]) {
    bool extractDone = false;
    ExtractSteps extractStep = ES_PORT_ARCHIVE;
    WindowsSteps windowsStep = WS_TEMP;
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetWindow());
    auto gui = wnd->GetGui();
    bool menuWasVisible = false;
    if (gui->GetMenu()->IsVisible()) {
        menuWasVisible = true;
        gui->GetMenu()->Hide();
    }

    OTRVersion romArchiveVersion = DetectOTRVersion("sm64.o2r");

    bool shouldRegen = !VerifyArchiveVersion(romArchiveVersion) && romArchiveVersion.major != INT16_MAX;

    std::filesystem::path ownPath;
    std::vector<std::string> args;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[argc]);
        }
    }
    GameExtractor extract;
    PromptSteps promptStep = PS_FILE_CHECK;
    std::atomic<bool> extracting = false;
    std::atomic<size_t> extractCount{ 0 }, totalExtract{ 0 };

    std::string installPath = Ship::Context::GetAppBundlePath();
    std::string file;

#if defined(__SWITCH__)
    GhostshipGui::RegisterPopup("Outdated ROM Archives",
                                "\x1b[2;2HYou've launched the Ship with an old ROM O2R file."
                                "\x1b[4;2HPlease regenerate a new ROM O2R and relaunch."
                                "\x1b[6;2HPress the Home button to exit...",
                                "OK", "", [&]() { exit(1); });
#elif defined(__WIIU__)
    GhostshipGui::RegisterPopup("Outdated ROM Archives",
                                "You've launched the Ship with an old a ROM O2R file.\n\n"
                                "Please generate a ROM O2R and relaunch.\n\n"
                                "Press and hold the Power button to shutdown...",
                                "OK", "", [&]() { exit(1); });
    OSFatal();
#endif

    if (!std::filesystem::exists(installPath + "/assets")) {
        GhostshipGui::RegisterPopup("Extractor assets not found",
                                    "No O2R files found. Missing 'assets/' folder needed to generate OTR file.\nPlease "
                                    "re-extract them from the download or.\n\nExiting...",
                                    "OK", "", [&]() {
                                        gsFast3dWindow = nullptr;
                                        context = nullptr;
                                        exit(1);
                                    });
    } else if (shouldRegen) {
        GhostshipGui::RegisterPopup("Outdated ROM Archives",
                                    "Your sm64.o2r was created with incompatible versions of Ghostship.\nYou will "
                                    "now be redirected to re-extract them.");
        std::filesystem::remove("sm64.o2r");
    }

    std::shared_ptr<BS::thread_pool> threadPool = std::make_shared<BS::thread_pool>(1);
    while (!extractDone) {
        if (GhostshipGui::PopupsQueued() > 0 || extracting) {
            goto render;
        }
        switch (extractStep) {
            case ES_PORT_ARCHIVE: {
                // if (portArchiveVersionMatch) {
#ifdef _WIN32
                extractStep = ES_WINDOWS;
#elif (defined(__WIIU__) || defined(__SWITCH__))
                extractStep = ES_VERIFY;
#else
                extractStep = ES_EXTRACT;
#endif
                /*} else {
                    std::string msg;

    #if defined(__SWITCH__)
                    msg = "\x1b[4;2HPlease re-extract it from the download.\n"
                        "\x1b[6;2HPress the Home button to exit...";
    #elif defined(__WIIU__)
                    msg = "Please extract the soh.o2r from the Ship of Harkinian download\nto your folder.\n\nPress "
                        "and hold the power\n"
                        "button to shutdown...";
    #else
                    msg =
                        "Please extract the soh.o2r from the Ship of Harkinian download to your folder.\n\nExiting...";
    #endif
                    std::string title =
                        !std::filesystem::exists(assets_path) ? "Missing ghostship.o2r" : "ghostship.o2r is outdated";
                    GhostshipGui::RegisterPopup(title, msg, "OK", "", [&]() { exit(1); });
                }
                continue;*/
            }
            case ES_WINDOWS: {
                switch (windowsStep) {
                    case WS_TEMP: {
#ifdef _WIN32
                        char* tempVar = getenv("TEMP");
                        std::filesystem::path tempPath;
                        try {
                            tempPath = std::filesystem::canonical(tempVar);
                        } catch (std::filesystem::filesystem_error const& ex) {
                            std::string userPath = getenv("USERPROFILE");
                            userPath.append("\\AppData\\Local\\Temp");
                            tempPath = std::filesystem::canonical(userPath);
                        }
                        wchar_t buffer[MAX_PATH];
                        GetModuleFileName(NULL, buffer, _countof(buffer));
                        ownPath = std::filesystem::canonical(buffer).parent_path();
                        if (IsSubpath(ownPath, tempPath)) {
                            GhostshipGui::RegisterPopup(
                                "Ghostship Path Error",
                                "Ghostship is running in a temp folder.\nExtract the .zip and run again.", "OK", "",
                                [&]() {
                                    threadPool = nullptr;
                                    gsFast3dWindow = nullptr;
                                    context = nullptr;
                                    exit(0);
                                });
                        } else {
                            windowsStep = WS_PERMS;
                        }
#endif
                        continue;
                    }
                    case WS_PERMS: {
                        FILE* tfile = fopen("./text.txt", "w");
                        std::filesystem::path tfolder = std::filesystem::path("./test/");
                        bool error = false;
                        try {
                            create_directories(tfolder);
                        } catch (std::filesystem::filesystem_error const& ex) { error = true; }
                        if (tfile == NULL || error) {
                            GhostshipGui::RegisterPopup(
                                "Ghostship Permissions Error",
                                "Ghostship does not have proper file permissions.\nPlease move it to a "
                                "folder that does and run again.",
                                "OK", "", [&]() {
                                    fclose(tfile);
                                    PathTestCleanup(tfile);
                                    threadPool = nullptr;
                                    gsFast3dWindow = nullptr;
                                    context = nullptr;
                                    exit(0);
                                });
                        } else {
                            fclose(tfile);
                            if (!PathTestCleanup(tfile)) {
                                GhostshipGui::RegisterPopup(
                                    "Ghostship Permissions Error",
                                    "Ghostship does not have proper file permissions.\nPlease move it to a "
                                    "folder that does and run again.",
                                    "OK", "", [&]() {
                                        threadPool = nullptr;
                                        gsFast3dWindow = nullptr;
                                        context = nullptr;
                                        exit(0);
                                    });
                            }
                            windowsStep = WS_ONEDRIVE;
                        }
                        continue;
                    }
                    case WS_ONEDRIVE: {
                        if (ownPath.string().find("OneDrive") != std::string::npos) {
                            GhostshipGui::RegisterPopup(
                                "Ghostship Path Error",
                                "Ghostship appears to be in a OneDrive folder, which will cause issues.\n"
                                "Please move it to a folder outside of OneDrive, like the root of a\n"
                                "drive (e.g. \"C:\\Games\\Ghostship\").",
                                "OK", "", [&]() {
                                    threadPool = nullptr;
                                    gsFast3dWindow = nullptr;
                                    context = nullptr;
                                    exit(0);
                                });
                        } else {
                            windowsStep = WS_DONE;
                            if (args.size() > 0) {
                                extractStep = ES_EXTRACT_ARGS;
                            } else {
                                extractStep = ES_EXTRACT;
                            }
                        }
                        continue;
                    }
                    default:
                        continue;
                }
                break;
            }
            case ES_EXTRACT_ARGS: {
#if !defined(__SWITCH__) && !defined(__WIIU__)
                if (args.size() == 0) {
                    GhostshipGui::RegisterPopup(
                        "Run Ghostship", "All files have been processed. Run Ghostship?", "Yes", "No",
                        [&]() {
                            if (!std::filesystem::exists(Ship::Context::GetAppDirectoryPath("sm64") + "/sm64.o2r")) {
                                extractStep = ES_EXTRACT;
                                promptStep = PS_FILE_CHECK;
                            } else {
                                extractStep = ES_VERIFY;
                            }
                        },
                        [&]() {
                            threadPool = nullptr;
                            gsFast3dWindow = nullptr;
                            context = nullptr;
                            exit(0);
                        });
                    break;
                }
                file = args.at(0);
                args.erase(args.begin());
                extract = GameExtractor();
                if (extract.RunStandalone(file)) {
                    bool doExtract = true;
                    std::string archive = "sm64.o2r";
                    if (std::filesystem::exists(Ship::Context::GetAppDirectoryPath("sm64") + "/" + archive)) {
                        std::string msg = "Archive for current ROM, " + archive + ", already exists.\nExtract again?";
                        GhostshipGui::RegisterPopup("Confirm Re-extract", msg.c_str(), "Yes", "No", [&]() {
                            extracting = true;
                            threadPool->submit_task([&]() -> void {
                                extract.Parse(totalExtract, "sm64");
                                extract.GenerateOTR(extractCount, "sm64");
                                extracting = false;
                                extractCount = totalExtract = 0;
                            });
                        });
                    } else {
                        extracting = true;
                        threadPool->submit_task([&]() -> void {
                            extract.Parse(totalExtract, "sm64");
                            extract.GenerateOTR(extractCount, "sm64");
                            extracting = false;
                            extractCount = totalExtract = 0;
                        });
                    }
                } else {
                    bool open = true;
                    std::string msg = "File\n" + std::string(file) + "\nis not a ROM or does not match supported ROMs.";
                    GhostshipGui::RegisterPopup("Ghostship ROM Error", msg.c_str());
                }
#else
                extractStep = ES_VERIFY;
#endif
                break;
            }
            case ES_EXTRACT: {
                switch (promptStep) {
                    case PS_FILE_CHECK: {
                        const bool romO2RExists =
                            std::filesystem::exists(Ship::Context::LocateFileAcrossAppDirs("sm64.o2r", "sm64"));

                        if (!romO2RExists) {
                            GhostshipGui::RegisterPopup(
                                "No O2R Files", "No O2R files found. Generate one now?", "Yes", "No",
                                [&]() { promptStep = PS_LOCAL; },
                                [&]() {
                                    threadPool = nullptr;
                                    gsFast3dWindow = nullptr;
                                    context = nullptr;
                                    exit(0);
                                });
                        } else {
                            extractStep = ES_VERIFY;
                        }
                        continue;
                    }
                    case PS_LOCAL: {
                        extract = GameExtractor();
                        extract.SetSearchPath(installPath);
                        extract.GetRoms(args);
                        extract.SetSearchPath(Ship::Context::GetAppDirectoryPath("sm64"));
                        extract.GetRoms(args);
                        if (!args.empty()) {
                            promptStep = PS_WAIT;
                            GhostshipGui::RegisterPopup(
                                "ROMs found", "ROMs found in application directory. Would you like to process them?",
                                "Yes", "No", [&]() { extractStep = ES_EXTRACT_ARGS; },
                                [&]() {
                                    args.clear();
                                    promptStep = PS_FIRST;
                                });
                        } else {
                            promptStep = PS_FIRST;
                        }
                        continue;
                    }
                    case PS_FIRST: {
                        if (args.empty() && !extract.SelectGameFromUI()) {
                            promptStep = PS_FILE_CHECK;
                            continue;
                        }
                        extracting = true;
                        file = extract.GetRomPath();
                        threadPool->submit_task([&]() -> void {
                            extract.Parse(totalExtract, "sm64");
                            extract.GenerateOTR(extractCount, "sm64");
                            extracting = false;
                            extractStep = ES_VERIFY;
                            extractCount = 0;
                            totalExtract = 0;
                        });
                        continue;
                    }
                    default:
                        break;
                }
                break;
            }
            case ES_VERIFY: {
                const bool romO2RExists =
                    std::filesystem::exists(Ship::Context::LocateFileAcrossAppDirs("sm64.o2r", "sm64"));

                if (!romO2RExists) {
                    GhostshipGui::RegisterPopup("No ROM Archive",
                                                "No ROM O2R file detected. Please generate a ROM O2R and relaunch.",
                                                "OK", "", [&]() {
                                                    threadPool = nullptr;
                                                    gsFast3dWindow = nullptr;
                                                    context = nullptr;
                                                    exit(0);
                                                });
                }
                extractDone = true;
                continue;
            }
            default:
                break;
        }

    render:
        if (!WindowIsRunning()) {
            threadPool = nullptr;
            gsFast3dWindow = nullptr;
            context = nullptr;
            exit(0);
        }
        // Process window events for resize, mouse, keyboard events
        wnd->HandleEvents();
        UIWidgets::Colors themeColor =
            static_cast<UIWidgets::Colors>(CVarGetInteger(CVAR_SETTING("Menu.Theme"), UIWidgets::Colors::LightBlue));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, UIWidgets::ColorValues.at(themeColor));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, UIWidgets::ColorValues.at(UIWidgets::Colors::DarkGray));

        // Skip dropped frames
        if (!wnd->IsFrameReady()) {
            continue;
        }
        gui->StartDraw();
        gsFast3dWindow->StartFrame();
        gsFast3dWindow->RunGuiOnly();
        if (extracting && !ImGui::IsPopupOpen("ROM Extraction")) {
            ImGui::OpenPopup("ROM Extraction");
        }
        if (extracting) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
            auto color = UIWidgets::ColorValues.at(THEME_COLOR);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.x, color.y, color.z, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
            if (ImGui::BeginPopupModal("ROM Extraction", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoSavedSettings)) {
                float progress = (totalExtract > 0.0f ? (float)extractCount / (float)totalExtract : 0) * 100.0f;
                auto filename = std::filesystem::path(file).filename().string();
                ImGui::Text("Extracting %s...%s", filename.c_str(),
                            roundf(progress) == 100.0f ? " Done. Finishing up." : "");
                std::string overlay = extractCount > 0 ? fmt::format("{:.0f}%", progress) : "Starting Up";
                ImGui::ProgressBar(progress / 100.0f, ImVec2(600.0f, 50.0f), overlay.c_str());
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }
        gui->EndDraw();
        gsFast3dWindow->EndFrame();
        ImGui::PopStyleColor(2);
    }
    threadPool = nullptr;

#ifdef __SWITCH__
    Ship::Switch::Init(Ship::PreInitPhase);
#elif defined(__WIIU__)
    Ship::WiiU::Init(appShortName);
#endif

#if not defined(__SWITCH__) && not defined(__WIIU__)
    CheckAndCreateModFolder();
#endif
    if (menuWasVisible) {
        gui->GetMenu()->Show();
    }
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

void GameEngine::Create(int argc, char* argv[]) {
    const auto instance = Instance = new GameEngine();
    instance->RunExtract(argc, argv);
    instance->FinishInit();
}

void GameEngine::Destroy() {
    GhostshipGui::Destroy();
    gsFast3dWindow = nullptr;
    Instance->context = nullptr;
    AudioExit();
#ifdef __SWITCH__
    Ship::Switch::Exit();
#endif
    for (auto& entry : Instance->memoryPool) {
        delete[] entry.addr;
    }
}

void GameEngine::StartFrame() const {
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
        case KbScancode::LUS_KB_TAB: {
            // Toggle HD Assets
            CVarSetInteger("gEnhancements.Mods.AlternateAssets",
                           !CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1));
            break;
        }
        default:
            break;
    }
}

uint32_t GameEngine::GetInterpolationFPS() {
    if (CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0)) {
        return Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
    } else if (CVarGetInteger(CVAR_VSYNC_ENABLED, 1) ||
               !Ship::Context::GetInstance()->GetWindow()->CanDisableVerticalSync()) {
        return std::min<uint32_t>(Ship::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate(),
                                  CVarGetInteger(CVAR_SETTING("InterpolationFPS"), 30));
    }
    return CVarGetInteger(CVAR_SETTING("InterpolationFPS"), 30);
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
        if(sequence.find(".m64") != std::string::npos) {
            continue;
        }
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

    bool curAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 1);
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

bool GameEngine::IsAltAssetsEnabled() {
    return prevAltAssets;
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

extern "C" bool GameEngine_OTRSigCheck(const char* data) {
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

std::wstring StringToU16(const std::string& s) {
    std::vector<unsigned long> result;
    size_t i = 0;

    while (i < s.size()) {
        unsigned long uni;
        size_t nbytes = 0;
        bool error = false;
        unsigned char c = s[i++];
        if (c < 0x80) { // ascii
            uni = c;
            nbytes = 0;
        } else if (c == GFXP_HIRAGANA_CHAR) { // Start Hiragana Mode
            uni = c;
            nbytes = 0;
        } else if (c == GFXP_KATAKANA_CHAR) { // Start Katakana Mode
            uni = c;
            nbytes = 0;
        } else if (c <= 0xBF) { // Invalid Characters (Skipped)
            nbytes = 0;
            uni = '\1';
        } else if (c <= 0xDF) {
            uni = c & 0x1F;
            nbytes = 1;
        } else if (c <= 0xEF) {
            uni = c & 0x0F;
            nbytes = 2;
        } else if (c <= 0xF7) {
            uni = c & 0x07;
            nbytes = 3;
        }
        for (size_t j = 0; j < nbytes; ++j) {
            unsigned char c = s[i++];
            uni <<= 6;
            uni += c & 0x3F;
        }
        if (uni != '\1')
            result.push_back(uni);
    }
    std::wstring utf16;
    for (size_t i = 0; i < result.size(); ++i) {
        unsigned long uni = result[i];
        if (uni <= 0xFFFF) {
            utf16 += (wchar_t)uni;
        } else {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return utf16;
}

extern "C" void GameEngine_GfxPrint(const char* str, void* printer, void (*printImpl)(void*, char)) {
    const std::vector<uint32_t> hira1 = {
        u'を', u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ', u'ゃ', u'ゅ', u'ょ', u'っ', u'-',  u'あ', u'い',
        u'う', u'え', u'お', u'か', u'き', u'く', u'け', u'こ', u'さ', u'し', u'す', u'せ', u'そ',
    };

    const std::vector<uint32_t> hira2 = {
        u'た', u'ち', u'つ', u'て', u'と', u'な', u'に', u'ぬ', u'ね', u'の', u'は', u'ひ', u'ふ', u'へ', u'ほ', u'ま',
        u'み', u'む', u'め', u'も', u'や', u'ゆ', u'よ', u'ら', u'り', u'る', u'れ', u'ろ', u'わ', u'ん', u'゛', u'゜',
    };

    const std::vector<uint32_t> kata1 = {
        u'ヲ', u'ァ', u'ィ', u'ゥ', u'ェ', u'ォ', u'ャ', u'ュ', u'ョ', u'ッ', u'ー',
    };

    const std::vector<uint32_t> kata2 = {
        u'ア', u'イ', u'ウ', u'エ', u'オ', u'カ', u'キ', u'ク', u'ケ', u'コ', u'サ', u'シ', u'ス', u'セ', u'ソ',
        u'タ', u'チ', u'ツ', u'テ', u'ト', u'ナ', u'ニ', u'ヌ', u'ネ', u'ノ', u'ハ', u'ヒ', u'フ', u'ヘ', u'ホ',
        u'マ', u'ミ', u'ム', u'メ', u'モ', u'ヤ', u'ユ', u'ヨ', u'ラ', u'リ', u'ル', u'レ', u'ロ', u'ワ', u'ン',
    };

    std::wstring wstr = StringToU16(str);
    bool hiraganaMode = false;

    for (const auto& c : wstr) {
        if (c < 0x80) {
            printImpl(printer, c);
        } else if (c == GFXP_HIRAGANA_CHAR) {
            hiraganaMode = true;
        } else if (c == GFXP_KATAKANA_CHAR) {
            hiraganaMode = false;
        } else if (c >= u'｡' && c <= u'ﾟ') { // katakana (hankaku)
            if (hiraganaMode && c >= u'ｦ' && c <= u'ｿ') {
                printImpl(printer, c - 0xFEC0 - 0x20); // Hiragana Mode, Block 1
            } else if (hiraganaMode && c >= u'ﾀ' && c <= u'ﾝ') {
                printImpl(printer, c - 0xFEC0 + 0x20); // Hiragana Mode, Block 2
            } else {
                printImpl(printer, c - 0xFEC0);
            }
        } else if (c == u'　') { // zenkaku space
            printImpl(printer, u' ');
        } else {
            auto it = std::find(hira1.begin(), hira1.end(), c);
            if (it != hira1.end()) { // hiragana block 1
                printImpl(printer, 0x86 + std::distance(hira1.begin(), it));
            }

            auto it2 = std::find(hira2.begin(), hira2.end(), c);
            if (it2 != hira2.end()) { // hiragana block 2
                printImpl(printer, 0xe0 + std::distance(hira2.begin(), it2));
            }

            auto it3 = std::find(kata1.begin(), kata1.end(), c);
            if (it3 != kata1.end()) { // katakana zenkaku block 1
                printImpl(printer, 0xa6 + std::distance(kata1.begin(), it3));
            }

            auto it4 = std::find(kata2.begin(), kata2.end(), c);
            if (it4 != kata2.end()) { // katakana zenkaku block 2
                printImpl(printer, 0xb1 + std::distance(kata2.begin(), it4));
            }
        }
    }
}

extern "C" void* GameEngine_GetExactDataByName(const char* path) {
    auto asset = Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(path, true);
    return asset ? static_cast<void*>(asset->GetRawPointer()) : nullptr;
}

extern "C" void* GameEngine_Malloc(size_t size) {
    auto& pool = GameEngine::Instance->memoryPool;

    pool.push_back({ new uint8_t[size], size });
    return (void*)pool.back().addr;
}

extern "C" void GameEngine_Free(void* ptr) {
    auto& pool = GameEngine::Instance->memoryPool;

    for (auto it = pool.begin(); it != pool.end(); ++it) {
        if (it->addr == ptr) {
            delete[] it->addr;
            pool.erase(it);
            break;
        }
    }
}