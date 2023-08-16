#include "ImguiUI.h"

#include <spdlog/spdlog.h>
#include <ImGui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui_internal.h>
#include <libultraship/libultraship.h>
#include <Fast3D/gfx_pc.h>

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