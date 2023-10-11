#include "ImguiUI.h"
#include "UIWidgets.h"
#include "ResolutionEditor.h"

#include <spdlog/spdlog.h>
#include <ImGui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui_internal.h>
#include <libultraship/libultraship.h>
#include <Fast3D/gfx_pc.h>
#include "port/Engine.h"
extern "C" {
#include "audio/external.h"
}

namespace GameUI {
std::shared_ptr<GameMenuBar> mGameMenuBar;
std::shared_ptr<LUS::GuiWindow> mConsoleWindow;
std::shared_ptr<LUS::GuiWindow> mStatsWindow;
std::shared_ptr<LUS::GuiWindow> mInputEditorWindow;
std::shared_ptr<AdvancedResolutionSettings::AdvancedResolutionSettingsWindow> mAdvancedResolutionSettingsWindow;

void SetupGuiElements() {
    auto gui = LUS::Context::GetInstance()->GetWindow()->GetGui();

    mGameMenuBar = std::make_shared<GameMenuBar>("gOpenMenuBar", CVarGetInteger("gOpenMenuBar", 0));
    gui->SetMenuBar(mGameMenuBar);
    mStatsWindow = gui->GetGuiWindow("Stats");
    if (mStatsWindow == nullptr) {
        SPDLOG_ERROR("Could not find stats window");
    }

    mConsoleWindow = gui->GetGuiWindow("Console");
    if (mConsoleWindow == nullptr) {
        SPDLOG_ERROR("Could not find console window");
    }

    mInputEditorWindow = gui->GetGuiWindow("Input Editor");
    if (mInputEditorWindow == nullptr) {
        SPDLOG_ERROR("Could not find input editor window");
        return;
    }

    mAdvancedResolutionSettingsWindow = std::make_shared<AdvancedResolutionSettings::AdvancedResolutionSettingsWindow>("gAdvancedResolutionEditorEnabled", "Advanced Resolution Settings");
    gui->AddGuiWindow(mAdvancedResolutionSettingsWindow);
}

void Destroy() {
    mAdvancedResolutionSettingsWindow = nullptr;
    mConsoleWindow = nullptr;
    mStatsWindow = nullptr;
    mInputEditorWindow = nullptr;
}

std::string GetWindowButtonText(const char* text, bool menuOpen) {
    char buttonText[100] = "";
    if (menuOpen) {
        strcat(buttonText, ICON_FA_CHEVRON_RIGHT " ");
    }
    strcat(buttonText, text);
    if (!menuOpen) { strcat(buttonText, "  "); }
    return buttonText;
}
}

static const char* filters[3] = {
#ifdef __WIIU__
        "",
#else
        "Three-Point",
#endif
        "Linear", "None"
};

void DrawSettingsMenu(){
    if(ImGui::BeginMenu("Settings")){
        if (ImGui::BeginMenu("Audio")) {
            UIWidgets::PaddedEnhancementSliderFloat("Master Volume: %d %%", "##Master_Vol", "gGameMasterVolume", 0.0f, 1.0f, "", 1.0f, true, true, false, true);
            if (UIWidgets::PaddedEnhancementSliderFloat("Main Music Volume: %d %%", "##Main_Music_Vol", "gMainMusicVolume", 0.0f, 1.0f, "", 1.0f, true, true, false, true)) {
                audio_set_player_volume(SEQ_PLAYER_LEVEL, CVarGetFloat("gMainMusicVolume", 1.0f));
            }
            if (UIWidgets::PaddedEnhancementSliderFloat("Sound Effects Volume: %d %%", "##Sound_Effect_Vol", "gSFXMusicVolume", 0.0f, 1.0f, "", 1.0f, true, true, false, true)) {
                audio_set_player_volume(SEQ_PLAYER_SFX, CVarGetFloat("gSFXMusicVolume", 1.0f));
            }
            if (UIWidgets::PaddedEnhancementSliderFloat("Environment Volume: %d %%", "##Environment_Vol", "gEnvironmentVolume", 0.0f, 1.0f, "", 1.0f, true, true, false, true)) {
                audio_set_player_volume(SEQ_PLAYER_ENV, CVarGetFloat("gEnvironmentVolume", 1.0f));
            }

            static std::unordered_map<LUS::AudioBackend, const char*> audioBackendNames = {
                    { LUS::AudioBackend::WASAPI, "Windows Audio Session API" },
                    { LUS::AudioBackend::PULSE, "PulseAudio" },
                    { LUS::AudioBackend::SDL, "SDL" },
            };

            ImGui::Text("Audio API (Needs reload)");
            auto currentAudioBackend = LUS::Context::GetInstance()->GetAudio()->GetAudioBackend();

            if (LUS::Context::GetInstance()->GetAudio()->GetAvailableAudioBackends()->size() <= 1) {
                UIWidgets::DisableComponent(ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::BeginCombo("##AApi", audioBackendNames[currentAudioBackend])) {
                for (uint8_t i = 0; i < LUS::Context::GetInstance()->GetAudio()->GetAvailableAudioBackends()->size(); i++) {
                    auto backend = LUS::Context::GetInstance()->GetAudio()->GetAvailableAudioBackends()->data()[i];
                    if (ImGui::Selectable(audioBackendNames[backend], backend == currentAudioBackend)) {
                        LUS::Context::GetInstance()->GetAudio()->SetAudioBackend(backend);
                    }
                }
                ImGui::EndCombo();
            }
            if (LUS::Context::GetInstance()->GetAudio()->GetAvailableAudioBackends()->size() <= 1) {
                UIWidgets::ReEnableComponent("");
            }

            ImGui::EndMenu();
        }

        UIWidgets::Spacer(0);

        if (ImGui::BeginMenu("Controller")) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2 (12.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
            if (GameUI::mInputEditorWindow) {
                if (ImGui::Button("Controller Mapping", ImVec2 (-1.0f, 0.0f))) {
                    GameUI::mInputEditorWindow->ToggleVisibility();
                }
            }
            UIWidgets::PaddedSeparator();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(3);
#ifndef __SWITCH__
            UIWidgets::EnhancementCheckbox("Menubar Controller Navigation", "gControlNav");
            UIWidgets::Tooltip("Allows controller navigation of the SOH menu bar (Settings, Enhancements,...)\nCAUTION: This will disable game inputs while the menubar is visible.\n\nD-pad to move between items, A to select, and X to grab focus on the menu bar");
#endif
            UIWidgets::PaddedEnhancementCheckbox("Show Inputs", "gInputEnabled", true, false);
            UIWidgets::Tooltip("Shows currently pressed inputs on the bottom right of the screen");
            UIWidgets::PaddedEnhancementSliderFloat("Input Scale: %.1f", "##Input", "gInputScale", 1.0f, 3.0f, "", 1.0f, false, true, true, false);
            UIWidgets::Tooltip("Sets the on screen size of the displayed inputs from the Show Inputs setting");

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    UIWidgets::Spacer(0);

    ImGui::SetCursorPosY(0.0f);
    if (ImGui::BeginMenu("Graphics")) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
        UIWidgets::Spacer(0);
        if (GameUI::mAdvancedResolutionSettingsWindow) {
            if (ImGui::Button(GameUI::GetWindowButtonText("Advanced Resolution", CVarGetInteger("gAdvancedResolutionEditorEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f))) {
                GameUI::mAdvancedResolutionSettingsWindow->ToggleVisibility();
            }
        }
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(3);

        UIWidgets::Tooltip("Multiplies your output resolution by the value inputted, as a more intensive but effective form of anti-aliasing");
        LUS::Context::GetInstance()->GetWindow()->SetResolutionMultiplier(CVarGetFloat("gInternalResolution", 1));
#ifndef __WIIU__
        UIWidgets::PaddedEnhancementSliderInt("MSAA: %d", "##IMSAA", "gMSAAValue", 1, 8, "", 1, true, true, false);
        UIWidgets::Tooltip("Activates multi-sample anti-aliasing when above 1x up to 8x for 8 samples for every pixel");
        LUS::Context::GetInstance()->GetWindow()->SetMsaaLevel(CVarGetInteger("gMSAAValue", 1));
#endif
        UIWidgets::Tooltip("Matches interpolation value to the current game's window refresh rate");

        { // FPS Slider
            const int minFps = 30;
            static int maxFps;
            if (LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() == LUS::WindowBackend::DX11) {
                maxFps = 360;
            } else {
                maxFps = LUS::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
            }
            int currentFps = fmax(fmin(GameEngine::GetInterpolationFPS(), maxFps), minFps);
        #ifdef __WIIU__
            UIWidgets::Spacer(0);
            // only support divisors of 60 on the Wii U
            if (currentFps > 60) {
                currentFps = 60;
            } else {
                currentFps = 60 / (60 / currentFps);
            }

            int fpsSlider = 1;
            if (currentFps == 30) {
                ImGui::Text("FPS: Original (30)");
            } else {
                ImGui::Text("FPS: %d", currentFps);
                if (currentFps == 30) {
                    fpsSlider = 2;
                } else { // currentFps == 60
                    fpsSlider = 3;
                }
            }
            if (CVarGetInteger("gMatchRefreshRate", 0)) {
                UIWidgets::DisableComponent(ImGui::GetStyle().Alpha * 0.5f);
            }

            if (ImGui::Button(" - ##WiiUFPS")) {
                fpsSlider--;
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);

            UIWidgets::Spacer(0);

            ImGui::PushItemWidth(std::min((ImGui::GetContentRegionAvail().x - 60.0f), 260.0f));
            ImGui::SliderInt("##WiiUFPSSlider", &fpsSlider, 1, 3, "", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
            if (ImGui::Button(" + ##WiiUFPS")) {
                fpsSlider++;
            }

            if (CVarGetInteger("gMatchRefreshRate", 0)) {
                UIWidgets::ReEnableComponent("");
            }
            if (fpsSlider > 3) {
                fpsSlider = 3;
            } else if (fpsSlider < 1) {
                fpsSlider = 1;
            }

            if (fpsSlider == 1) {
                currentFps = 20;
            } else if (fpsSlider == 2) {
                currentFps = 30;
            } else if (fpsSlider == 3) {
                currentFps = 60;
            }
            CVarSetInteger("gInterpolationFPS", currentFps);
            LUS::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesOnNextTick();
        #else
            bool matchingRefreshRate =
                CVarGetInteger("gMatchRefreshRate", 0) && LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() != LUS::WindowBackend::DX11;
            UIWidgets::PaddedEnhancementSliderInt(
                (currentFps == 20) ? "FPS: Original (20)" : "FPS: %d",
                "##FPSInterpolation", "gInterpolationFPS", minFps, maxFps, "", 20, true, true, false, matchingRefreshRate);
        #endif
            if (LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() == LUS::WindowBackend::DX11) {
                UIWidgets::Tooltip(
                    "Uses Matrix Interpolation to create extra frames, resulting in smoother graphics. This is purely "
                    "visual and does not impact game logic, execution of glitches etc.\n\n"
                    "A higher target FPS than your monitor's refresh rate will waste resources, and might give a worse result."
                );
            } else {
                UIWidgets::Tooltip(
                    "Uses Matrix Interpolation to create extra frames, resulting in smoother graphics. This is purely "
                    "visual and does not impact game logic, execution of glitches etc."
                );
            }
        } // END FPS Slider

        if (LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() == LUS::WindowBackend::DX11) {
            UIWidgets::Spacer(0);
            if (ImGui::Button("Match Refresh Rate")) {
                int hz = LUS::Context::GetInstance()->GetWindow()->GetCurrentRefreshRate();
                if (hz >= 30 && hz <= 360) {
                    CVarSetInteger("gInterpolationFPS", hz);
                    LUS::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesOnNextTick();
                }
            }
        } else {
            UIWidgets::PaddedEnhancementCheckbox("Match Refresh Rate", "gMatchRefreshRate", true, false);
        }

        UIWidgets::Tooltip("Matches interpolation value to the current game's window refresh rate");

        if (LUS::Context::GetInstance()->GetWindow()->GetWindowBackend() == LUS::WindowBackend::DX11) {
            UIWidgets::PaddedEnhancementSliderInt(CVarGetInteger("gExtraLatencyThreshold", 80) == 0 ? "Jitter fix: Off" : "Jitter fix: >= %d FPS",
                                                  "##ExtraLatencyThreshold", "gExtraLatencyThreshold", 0, 360, "", 80, true, true, false);
            UIWidgets::Tooltip("When Interpolation FPS setting is at least this threshold, add one frame of input lag (e.g. 16.6 ms for 60 FPS) in order to avoid jitter. This setting allows the CPU to work on one frame while GPU works on the previous frame.\nThis setting should be used when your computer is too slow to do CPU + GPU work in time.");
        }

        UIWidgets::PaddedSeparator(true, true, 3.0f, 3.0f);


        static std::unordered_map<LUS::WindowBackend, const char*> windowBackendNames = {
                { LUS::WindowBackend::DX11, "DirectX" },
                { LUS::WindowBackend::SDL_OPENGL, "OpenGL"},
                { LUS::WindowBackend::SDL_METAL, "Metal" },
                { LUS::WindowBackend::GX2, "GX2"}
        };

        ImGui::Text("Renderer API (Needs reload)");
        LUS::WindowBackend runningWindowBackend = LUS::Context::GetInstance()->GetWindow()->GetWindowBackend();
        LUS::WindowBackend configWindowBackend;
        int configWindowBackendId = LUS::Context::GetInstance()->GetConfig()->GetInt("Window.Backend.Id", -1);
        if (configWindowBackendId != -1 && configWindowBackendId < static_cast<int>(LUS::WindowBackend::BACKEND_COUNT)) {
            configWindowBackend = static_cast<LUS::WindowBackend>(configWindowBackendId);
        } else {
            configWindowBackend = runningWindowBackend;
        }

        if (LUS::Context::GetInstance()->GetWindow()->GetAvailableWindowBackends()->size() <= 1) {
            UIWidgets::DisableComponent(ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::BeginCombo("##RApi", windowBackendNames[configWindowBackend])) {
            for (size_t i = 0; i < LUS::Context::GetInstance()->GetWindow()->GetAvailableWindowBackends()->size(); i++) {
                auto backend = LUS::Context::GetInstance()->GetWindow()->GetAvailableWindowBackends()->data()[i];
                if (ImGui::Selectable(windowBackendNames[backend], backend == configWindowBackend)) {
                    LUS::Context::GetInstance()->GetConfig()->SetInt("Window.Backend.Id", static_cast<int>(backend));
                    LUS::Context::GetInstance()->GetConfig()->SetString("Window.Backend.Name",
                                                                        windowBackendNames[backend]);
                    LUS::Context::GetInstance()->GetConfig()->Save();
                }
            }
            ImGui::EndCombo();
        }
        if (LUS::Context::GetInstance()->GetWindow()->GetAvailableWindowBackends()->size() <= 1) {
            UIWidgets::ReEnableComponent("");
        }

        if (LUS::Context::GetInstance()->GetWindow()->CanDisableVerticalSync()) {
            UIWidgets::PaddedEnhancementCheckbox("Enable Vsync", "gVsyncEnabled", true, false);
        }

        if (LUS::Context::GetInstance()->GetWindow()->SupportsWindowedFullscreen()) {
            UIWidgets::PaddedEnhancementCheckbox("Windowed fullscreen", "gSdlWindowedFullscreen", true, false);
        }

        if (LUS::Context::GetInstance()->GetWindow()->GetGui()->SupportsViewports()) {
            UIWidgets::PaddedEnhancementCheckbox("Allow multi-windows", "gEnableMultiViewports", true, false, false, "", UIWidgets::CheckboxGraphics::Cross, true);
            UIWidgets::Tooltip("Allows windows to be able to be dragged off of the main game window. Requires a reload to take effect.");
        }

        // If more filters are added to LUS, make sure to add them to the filters list here
        ImGui::Text("Texture Filter (Needs reload)");

        UIWidgets::EnhancementCombobox("gTextureFilter", filters, 0);

        UIWidgets::Spacer(0);

        LUS::Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->DrawSettings();

        ImGui::EndMenu();
    }
}

void DrawMenuBarIcon() {
    static bool gameIconLoaded = false;
    if (!gameIconLoaded) {
        LUS::Context::GetInstance()->GetWindow()->GetGui()->LoadTexture("Game_Icon", "textures/icons/gIcon.png");
        gameIconLoaded = true;
    }

    if (LUS::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName("Game_Icon")) {
#ifdef __SWITCH__
        ImVec2 iconSize = ImVec2(20.0f, 20.0f);
        float posScale = 1.0f;
#elif defined(__WIIU__)
        ImVec2 iconSize = ImVec2(16.0f * 2, 16.0f * 2);
        float posScale = 2.0f;
#else
        ImVec2 iconSize = ImVec2(16.0f, 16.0f);
        float posScale = 1.0f;
#endif
        ImGui::SetCursorPos(ImVec2(5, 2.5f) * posScale);
        ImGui::Image(LUS::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName("Game_Icon"), iconSize);
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(25, 0) * posScale);
    }
}

void DrawGameMenu() {
    if (ImGui::BeginMenu("Ghostship")) {
        if (ImGui::MenuItem("Reset",
#ifdef __APPLE__
                "Command-R"
#else
                "Ctrl+R"
#endif
        )) {
            std::reinterpret_pointer_cast<LUS::ConsoleWindow>(LUS::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))->Dispatch("reset");
        }
#if !defined(__SWITCH__) && !defined(__WIIU__)

        if (ImGui::MenuItem("Toggle Fullscreen", "F9")) {
            LUS::Context::GetInstance()->GetWindow()->ToggleFullscreen();
        }

        if (ImGui::MenuItem("Quit")) {
            LUS::Context::GetInstance()->GetWindow()->Close();
        }
#endif
        ImGui::EndMenu();
    }
}

void DrawEnhancementsMenu() {
    if (ImGui::BeginMenu("Enhancements")) {

        if (ImGui::BeginMenu("Gameplay")) {
            UIWidgets::PaddedEnhancementCheckbox("No Level of Detail (LOD)", "gDisableLOD", true, false);
            UIWidgets::Tooltip("Disable Level of Detail (LOD) to avoid models using lower poly versions at a distance");
            UIWidgets::PaddedEnhancementCheckbox("Select any star from menu", "gSelectAllStars", true, false);
            UIWidgets::Tooltip("Let's you select any star from the menu regardless of the courses completion status.");
            UIWidgets::PaddedEnhancementCheckbox("Collecting Stars Will Not Exit Level", "gStarNoExit", true, false);
            UIWidgets::Tooltip("Stars act like the 100 coin star and will not take you out of the level");
            UIWidgets::PaddedEnhancementCheckbox("Avoid playing peach cutscene", "gDisablePeachCutscene", true, false);
            UIWidgets::Tooltip("Avoid playing the peach cutscene when starting a new game");
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

void DrawCheatsMenu() {
    if (ImGui::BeginMenu("Cheats")) {
        UIWidgets::PaddedEnhancementCheckbox("Infinite Health", "gInfiniteHealth", true, false);
        UIWidgets::PaddedEnhancementCheckbox("Infinite Lives", "gInfiniteLives", true, false);


        ImGui::EndMenu();
    }
}

void DrawDebugMenu() {
    if (ImGui::BeginMenu("Developer")) {

        UIWidgets::PaddedEnhancementCheckbox("Enable level selector", "gEnableDebugMode", true, false);
        UIWidgets::Tooltip("Enable the level selector in the main menu");
        UIWidgets::Spacer(0);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
        if (GameUI::mStatsWindow) {
            if (ImGui::Button(
                    GameUI::GetWindowButtonText("Stats", CVarGetInteger("gStatsEnabled", 0)).c_str(),
                    ImVec2(-1.0f, 0.0f))) {
                GameUI::mStatsWindow->ToggleVisibility();
            }
            UIWidgets::Tooltip("Shows the stats window, with your FPS and frametimes, and the OS "
                               "you're playing on");
        }
        if (GameUI::mConsoleWindow) {
            if (ImGui::Button(
                    GameUI::GetWindowButtonText("Console", CVarGetInteger("gConsoleEnabled", 0))
                        .c_str(),
                    ImVec2(-1.0f, 0.0f))) {
                GameUI::mConsoleWindow->ToggleVisibility();
            }
            UIWidgets::Tooltip("Enables the console window, allowing you to input commands, type "
                               "help for some examples");
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(1);

        ImGui::EndMenu();
    }
}

void GameMenuBar::DrawElement() {
    if(ImGui::BeginMenuBar()){
        DrawMenuBarIcon();

        static ImVec2 sWindowPadding(8.0f, 8.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, sWindowPadding);

        DrawGameMenu();

        ImGui::SetCursorPosY(0.0f);

        DrawSettingsMenu();

        ImGui::SetCursorPosY(0.0f);

        DrawEnhancementsMenu();

        ImGui::SetCursorPosY(0.0f);

        DrawCheatsMenu();

        ImGui::SetCursorPosY(0.0f);
        
        DrawDebugMenu();

        ImGui::PopStyleVar(1);
        ImGui::EndMenuBar();
    }
}