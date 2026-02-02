#include "GhostshipGui.hpp"

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>

#ifdef __APPLE__
#include <fast/backends/gfx_metal.h>
#endif

#ifdef __SWITCH__
#include <port/switch/SwitchImpl.h>
#endif

#include "Notification.h"
#include "GhostshipInputEditorWindow.h"
#include "SaveEditor.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/Rando/EntranceTracker/EntranceTracker.h"

namespace GhostshipGui {
// MARK: - Delegates
std::shared_ptr<Ship::GuiWindow> mInputEditorWindow;
std::shared_ptr<GhostshipMenu> mGhostshipMenu;
std::shared_ptr<SaveEditorWindow> mSaveEditorWindow;
std::shared_ptr<Notification::Window> mNotificationWindow;
std::shared_ptr<InputViewer> mInputViewer;
std::shared_ptr<InputViewerSettingsWindow> mInputViewerSettings;
std::shared_ptr<GhostshipModalWindow> mModalWindow;

std::shared_ptr<Rando::CheckTracker::CheckTrackerWindow> mRandoCheckTrackerWindow;
std::shared_ptr<Rando::CheckTracker::SettingsWindow> mRandoCheckTrackerSettingsWindow;

std::shared_ptr<Rando::EntranceTracker::EntranceTrackerWindow> mRandoEntranceTrackerWindow;
std::shared_ptr<Rando::EntranceTracker::SettingsWindow> mRandoEntranceTrackerSettingsWindow;

UIWidgets::Colors GetMenuThemeColor() {
    return mGhostshipMenu->GetMenuThemeColor();
}

void SetupGuiElements() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();

    auto& style = ImGui::GetStyle();
    style.FramePadding = ImVec2(4.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.Colors[ImGuiCol_MenuBarBg] = UIWidgets::ColorValues.at(UIWidgets::Colors::DarkGray);

    mGhostshipMenu = std::make_shared<GhostshipMenu>(CVAR_WINDOW("Menu"), "Settings Menu");
    gui->SetMenu(mGhostshipMenu);

    mSaveEditorWindow = std::make_shared<SaveEditorWindow>(CVAR_WINDOW("SaveEditor"), "Save Editor");
    gui->AddGuiWindow(mSaveEditorWindow);

    mInputEditorWindow =
        std::make_shared<GhostshipInputEditorWindow>(CVAR_WINDOW("ControllerConfiguration"), "Configure Controller");
    gui->AddGuiWindow(mInputEditorWindow);

    mNotificationWindow = std::make_shared<Notification::Window>(CVAR_WINDOW("Notifications"), "Notifications Window");
    gui->AddGuiWindow(mNotificationWindow);
    mNotificationWindow->Show();

    mInputViewer = std::make_shared<InputViewer>(CVAR_WINDOW("InputViewer"), "Input Viewer");
    gui->AddGuiWindow(mInputViewer);
    mInputViewerSettings = std::make_shared<InputViewerSettingsWindow>(CVAR_WINDOW("InputViewerSettings"),
                                                                       "Input Viewer Settings", ImVec2(500, 525));
    gui->AddGuiWindow(mInputViewerSettings);
    mModalWindow = std::make_shared<GhostshipModalWindow>(CVAR_WINDOW("ModalWindow"), "Modal Window");
    gui->AddGuiWindow(mModalWindow);
    mModalWindow->Show();

    mRandoCheckTrackerWindow = std::make_shared<Rando::CheckTracker::CheckTrackerWindow>(
        "gWindows.CheckTracker", "Check Tracker", ImVec2(375, 460));
    gui->AddGuiWindow(mRandoCheckTrackerWindow);

    mRandoCheckTrackerSettingsWindow = std::make_shared<Rando::CheckTracker::SettingsWindow>(
        "gWindows.CheckTrackerSettings", "Check Tracker Settings");
    gui->AddGuiWindow(mRandoCheckTrackerSettingsWindow);

    mRandoEntranceTrackerWindow = std::make_shared<Rando::EntranceTracker::EntranceTrackerWindow>(
        "gWindows.EntranceTracker", "Entrance Tracker", ImVec2(375, 460));
    gui->AddGuiWindow(mRandoEntranceTrackerWindow);

    mRandoEntranceTrackerSettingsWindow = std::make_shared<Rando::EntranceTracker::SettingsWindow>(
        "gWindows.EntranceTrackerSettings", "Entrance Tracker Settings");
    gui->AddGuiWindow(mRandoEntranceTrackerSettingsWindow);
}

void Destroy() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();

    gui->RemoveAllGuiWindows();
    mGhostshipMenu = nullptr;
    mModalWindow = nullptr;
    mSaveEditorWindow = nullptr;
    mInputEditorWindow = nullptr;
    mNotificationWindow = nullptr;
    mInputViewer = nullptr;
    mInputViewerSettings = nullptr;
    mRandoCheckTrackerWindow = nullptr;
    mRandoCheckTrackerSettingsWindow = nullptr;
    mRandoEntranceTrackerWindow = nullptr;
    mRandoEntranceTrackerSettingsWindow = nullptr;
}

void RegisterPopup(std::string title, std::string message, std::string button1, std::string button2,
                   std::function<void()> button1callback, std::function<void()> button2callback) {
    mModalWindow->RegisterPopup(title, message, button1, button2, button1callback, button2callback);
}

} // namespace GhostshipGui
