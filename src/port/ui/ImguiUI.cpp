#include "ImguiUI.h"
#include "UIWidgets.h"

#include <spdlog/spdlog.h>
#include <ImGui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui_internal.h>
#include <libultraship/libultraship.h>
#include <Fast3D/gfx_pc.h>
extern "C" {
#include "audio/external.h"
}

namespace GameUI {
std::shared_ptr<GameMenuBar> mGameMenuBar;
std::shared_ptr<LUS::GuiWindow> mConsoleWindow;
std::shared_ptr<LUS::GuiWindow> mStatsWindow;
std::shared_ptr<LUS::GuiWindow> mInputEditorWindow;

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
}

void Destroy() {
    mConsoleWindow = nullptr;
    mStatsWindow = nullptr;
    mInputEditorWindow = nullptr;
}
}

void GameMenuBar::DrawElement() {
    if(ImGui::BeginMenuBar()){
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

            if (GameUI::mInputEditorWindow) {
                if (ImGui::Button("Controller Mapping", ImVec2 (-1.0f, 0.0f))) {
                    GameUI::mInputEditorWindow->ToggleVisibility();
                }
            }
            LUS::Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->DrawSettings();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}