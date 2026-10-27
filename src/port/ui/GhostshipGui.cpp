#include "GhostshipGui.hpp"
#include "port/ShipCompat.h"

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "port/api/ui.h"

#ifdef ENABLE_SCRIPTING
void C_RunGuiDrawCallbacks();
#endif

#ifdef __APPLE__
#include <fast/backends/gfx_metal.h>
#endif

#include "Notification.h"
#include "TouchControls.h"
#include "GhostshipInputEditorWindow.h"
#include "SaveEditor.h"
#include "port/ui/ObjectViewer.h"
#include "port/ui/AchievementsWindow.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/Rando/EntranceTracker/EntranceTracker.h"

#include <ship/window/gui/ConsoleWindow.h>
#include "port/console/LogFeed.h"
#include <ship/window/gui/EventDebuggerWindow.h>
#if __has_include(<ship/window/gui/ShaderSettingsWindow.h>)
#include <ship/window/gui/ShaderSettingsWindow.h>
#define GHOSTSHIP_HAS_SHADER_SETTINGS 1
#endif
#include <libultraship/window/gui/GfxDebuggerWindow.h>

#ifdef ENABLE_SCRIPTING
// Invisible host window that fires C mod ImGui callbacks each frame.
// Overrides Draw() so it never calls ImGui::Begin()/End() itself; the C
// callbacks are free to open any number of their own igBegin/igEnd windows.
class CGuiCallbackWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
    void Draw() override {
        C_RunGuiDrawCallbacks();
    }
};
#endif

// Invisible window that moves new log messages into the console every frame, opened or not.
class LogFeedWindow : public Ship::GuiWindow {
  public:
    LogFeedWindow(const std::string& cvar, const std::string& name, std::shared_ptr<Ship::GuiWindow> console)
        : GuiWindow(cvar, name), mConsole(std::move(console)) {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
    void Draw() override {
        if (auto console = std::dynamic_pointer_cast<Ship::ConsoleWindow>(mConsole)) {
            LogFeed::Pump(*console);
        }
    }

  private:
    std::shared_ptr<Ship::GuiWindow> mConsole;
};

namespace GhostshipGui {
// MARK: - Delegates
std::shared_ptr<Ship::GuiWindow> mInputEditorWindow;
std::shared_ptr<GhostshipMenu> mGhostshipMenu;
std::shared_ptr<SaveEditorWindow> mSaveEditorWindow;
std::shared_ptr<Notification::Window> mNotificationWindow;
std::shared_ptr<InputViewer> mInputViewer;
std::shared_ptr<TouchControlsOverlay> mTouchControlsOverlay;
std::shared_ptr<InputViewerSettingsWindow> mInputViewerSettings;
std::shared_ptr<GhostshipModalWindow> mModalWindow;
std::shared_ptr<ObjectViewer> mObjectViewer;
std::shared_ptr<Ship::GuiWindow> mConsoleWindow;
std::shared_ptr<AchievementsWindow> mAchievementsWindow;
std::shared_ptr<Ship::EventDebuggerWindow> mEventDebuggerWindow;
std::shared_ptr<LUS::GfxDebuggerWindow> mGfxDebuggerWindow;
#ifdef GHOSTSHIP_HAS_SHADER_SETTINGS
std::shared_ptr<Ship::ShaderSettingsWindow> mShaderSettingsWindow;
#endif

std::shared_ptr<Rando::CheckTracker::CheckTrackerWindow> mRandoCheckTrackerWindow;
std::shared_ptr<Rando::CheckTracker::SettingsWindow> mRandoCheckTrackerSettingsWindow;

std::shared_ptr<Rando::EntranceTracker::EntranceTrackerWindow> mRandoEntranceTrackerWindow;
std::shared_ptr<Rando::EntranceTracker::SettingsWindow> mRandoEntranceTrackerSettingsWindow;

UIWidgets::Colors GetMenuThemeColor() {
    return mGhostshipMenu->GetMenuThemeColor();
}

void SetupMenu() {
    auto gui = ShipCompat::GetWindow()->GetGui();
    mGhostshipMenu = std::make_shared<GhostshipGui::GhostshipMenu>(CVAR_WINDOW("Menu"), "Port Menu");
    gui->SetMenu(mGhostshipMenu);

    mModalWindow = std::make_shared<GhostshipModalWindow>(CVAR_WINDOW("ModalWindow"), "Modal Window");
    gui->AddGuiWindow(mModalWindow);
    mModalWindow->Show();
}

void SetupGuiElements() {
    auto gui = ShipCompat::GetWindow()->GetGui();

    auto& style = ImGui::GetStyle();
    style.FramePadding = ImVec2(4.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.Colors[ImGuiCol_MenuBarBg] = UIWidgets::ColorValues.at(UIWidgets::Colors::DarkGray);

    mConsoleWindow = std::make_shared<Ship::ConsoleWindow>(CVAR_WINDOW("DevConsole"), "Console##Dev", ImVec2(820, 630));
    gui->AddGuiWindow(mConsoleWindow);
    // The console only offers its Logs channel while this is set; logs are shown unless the player turned it off.
    if (CVarGetInteger("gSinkEnabled", -1) == -1) {
        CVarSetInteger("gSinkEnabled", 1);
    }
    auto logFeedWindow = std::make_shared<LogFeedWindow>("gWindows.LogFeed", "##logfeed", mConsoleWindow);
    logFeedWindow->Show();
    gui->AddGuiWindow(logFeedWindow);

    mAchievementsWindow =
        std::make_shared<AchievementsWindow>(CVAR_WINDOW("Achievements"), "Achievements", ImVec2(960, 720));
    gui->AddGuiWindow(mAchievementsWindow);

    mEventDebuggerWindow = std::make_shared<Ship::EventDebuggerWindow>(CVAR_WINDOW("EventDebugger"), "Event Debugger");
    gui->AddGuiWindow(mEventDebuggerWindow);

    mGfxDebuggerWindow =
        std::make_shared<LUS::GfxDebuggerWindow>(CVAR_WINDOW("GfxDebugger"), "Gfx Debugger",
                                                 std::dynamic_pointer_cast<Fast::Fast3dWindow>(ShipCompat::GetWindow()),
                                                 ShipCompat::GetGfxDebugger(), ShipCompat::GetResourceManager());
    gui->AddGuiWindow(mGfxDebuggerWindow);

    mObjectViewer = std::make_shared<ObjectViewer>(CVAR_WINDOW("ObjectViewer"), "Object Viewer##Dev", ImVec2(820, 630));
    gui->AddGuiWindow(mObjectViewer);

#ifdef ENABLE_SCRIPTING
    auto cGuiWindow = std::make_shared<CGuiCallbackWindow>("gWindows.CGuiCallbacks", "##cguicallbacks");
    cGuiWindow->Show();
    gui->AddGuiWindow(cGuiWindow);
#endif

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

    mTouchControlsOverlay = std::make_shared<TouchControlsOverlay>(CVAR_WINDOW("TouchControls"), "##TouchControls");
    mTouchControlsOverlay->Show();
    gui->AddGuiWindow(mTouchControlsOverlay);
    mInputViewerSettings = std::make_shared<InputViewerSettingsWindow>(CVAR_WINDOW("InputViewerSettings"),
                                                                       "Input Viewer Settings", ImVec2(500, 525));
    gui->AddGuiWindow(mInputViewerSettings);

#ifdef GHOSTSHIP_HAS_SHADER_SETTINGS
    mShaderSettingsWindow = std::make_shared<Ship::ShaderSettingsWindow>(CVAR_WINDOW("ShaderSettings"),
                                                                         "Shader Settings", ImVec2(420, 520));
    gui->AddGuiWindow(mShaderSettingsWindow);
#endif

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
    auto gui = ShipCompat::GetWindow()->GetGui();

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
    mConsoleWindow = nullptr;
    mObjectViewer = nullptr;
#ifdef GHOSTSHIP_HAS_SHADER_SETTINGS
    mShaderSettingsWindow = nullptr;
#endif
    mGfxDebuggerWindow = nullptr;
    mTouchControlsOverlay = nullptr;
    mAchievementsWindow = nullptr;
    mEventDebuggerWindow = nullptr;
}

void OpenLogs(spdlog::level::level_enum minLevel) {
    auto console = std::dynamic_pointer_cast<Ship::ConsoleWindow>(mConsoleWindow);
    if (console == nullptr) {
        return;
    }
    CVarSetInteger("gSinkEnabled", 1);
    console->SetCurrentChannel("Logs");
    console->SetLevelFilter(minLevel);
    console->Show();
}

void RegisterPopup(std::string title, std::string message, std::string button1, std::string button2,
                   std::function<void()> button1callback, std::function<void()> button2callback) {
    mModalWindow->RegisterPopup(title, message, button1, button2, button1callback, button2callback);
}

size_t PopupsQueued() {
    return mModalWindow->PopupsQueued();
}

} // namespace GhostshipGui
