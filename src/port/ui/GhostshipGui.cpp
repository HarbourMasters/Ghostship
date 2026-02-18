#include "GhostshipGui.hpp"

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>

#ifdef __APPLE__
#include <fast/backends/gfx_metal.h>
#endif

#include "Notification.h"
#include "GhostshipInputEditorWindow.h"
#include "SaveEditor.h"

#include <ship/window/gui/ConsoleWindow.h>

namespace GhostshipGui {
// MARK: - Delegates
std::shared_ptr<Ship::GuiWindow> mInputEditorWindow;
std::shared_ptr<GhostshipMenu> mGhostshipMenu;
std::shared_ptr<SaveEditorWindow> mSaveEditorWindow;
std::shared_ptr<Notification::Window> mNotificationWindow;
std::shared_ptr<InputViewer> mInputViewer;
std::shared_ptr<InputViewerSettingsWindow> mInputViewerSettings;
std::shared_ptr<GhostshipModalWindow> mModalWindow;
std::shared_ptr<Ship::GuiWindow> mConsoleWindow;

UIWidgets::Colors GetMenuThemeColor() {
    return mGhostshipMenu->GetMenuThemeColor();
}

void SetupMenu() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    mGhostshipMenu = std::make_shared<GhostshipGui::GhostshipMenu>(CVAR_WINDOW("Menu"), "Port Menu");
    gui->SetMenu(mGhostshipMenu);

    mModalWindow = std::make_shared<GhostshipModalWindow>(CVAR_WINDOW("ModalWindow"), "Modal Window");
    gui->AddGuiWindow(mModalWindow);
    mModalWindow->Show();
}

void SetupGuiElements() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();

    auto& style = ImGui::GetStyle();
    style.FramePadding = ImVec2(4.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.Colors[ImGuiCol_MenuBarBg] = UIWidgets::ColorValues.at(UIWidgets::Colors::DarkGray);

    mConsoleWindow = std::make_shared<Ship::ConsoleWindow>(CVAR_WINDOW("DevConsole"), "Console##Dev", ImVec2(820, 630));
    gui->AddGuiWindow(mConsoleWindow);

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
    mConsoleWindow = nullptr;
}

void RegisterPopup(std::string title, std::string message, std::string button1, std::string button2,
                   std::function<void()> button1callback, std::function<void()> button2callback) {
    mModalWindow->RegisterPopup(title, message, button1, button2, button1callback, button2callback);
}

size_t PopupsQueued() {
    return mModalWindow->PopupsQueued();
}

} // namespace GhostshipGui
