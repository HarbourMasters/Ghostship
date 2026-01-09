#include "GhostshipModals.h"
#include <imgui.h>
#include <vector>
#include <string>
#include "UIWidgets.hpp"
#include "GhostshipGui.hpp"

struct GhostshipModal {
    std::string title_;
    std::string message_;
    std::string button1_;
    std::string button2_;
    std::function<void()> button1callback_;
    std::function<void()> button2callback_;
};
std::vector<GhostshipModal> modals;

bool closePopup = false;

void GhostshipModalWindow::Draw() {
    if (!IsVisible()) {
        return;
    }
    DrawElement();
    // Sync up the IsVisible flag if it was changed by ImGui
    SyncVisibilityConsoleVariable();
}

void GhostshipModalWindow::DrawElement() {
    if (modals.size() > 0) {
        GhostshipModal curModal = modals.at(0);
        if (!ImGui::IsPopupOpen(curModal.title_.c_str())) {
            ImGui::OpenPopup(curModal.title_.c_str());
        }
        if (closePopup) {
            ImGui::CloseCurrentPopup();
            modals.erase(modals.begin());
            closePopup = false;
        }
        if (ImGui::BeginPopupModal(curModal.title_.c_str(), NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::Text("%s", curModal.message_.c_str());
            UIWidgets::PushStyleButton(THEME_COLOR);
            if (ImGui::Button(curModal.button1_.c_str())) {
                if (curModal.button1callback_ != nullptr) {
                    curModal.button1callback_();
                }
                ImGui::CloseCurrentPopup();
                modals.erase(modals.begin());
            }
            UIWidgets::PopStyleButton();
            if (curModal.button2_ != "") {
                ImGui::SameLine();
                UIWidgets::PushStyleButton(THEME_COLOR);
                if (ImGui::Button(curModal.button2_.c_str())) {
                    if (curModal.button2callback_ != nullptr) {
                        curModal.button2callback_();
                    }
                    ImGui::CloseCurrentPopup();
                    modals.erase(modals.begin());
                }
                UIWidgets::PopStyleButton();
            }
        }
        ImGui::EndPopup();
    }
}

void GhostshipModalWindow::RegisterPopup(std::string title, std::string message, std::string button1,
                                          std::string button2, std::function<void()> button1callback,
                                          std::function<void()> button2callback) {
    modals.push_back({ title, message, button1, button2, button1callback, button2callback });
}

bool GhostshipModalWindow::IsPopupOpen(std::string title) {
    return !modals.empty() && modals.at(0).title_ == title;
}

void GhostshipModalWindow::DismissPopup() {
    closePopup = true;
}
