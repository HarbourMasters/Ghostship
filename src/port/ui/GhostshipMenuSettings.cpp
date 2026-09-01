#include "GhostshipMenu.h"
#include "port/ShipCompat.h"
#include "port/Engine.h"
#include "Notification.h"
#include "GhostshipInputEditorWindow.h"
#include "GhostshipModals.h"
#include "TouchControls.h"
#include "UIWidgets.hpp"
#include <spdlog/fmt/fmt.h>

extern "C" {
#include "variables.h"
#include "audio/external.h"
}

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
extern std::shared_ptr<GhostshipModalWindow> mModalWindow;
using namespace UIWidgets;

static std::unordered_map<int32_t, const char*> imguiScaleOptions = {
    { 0, "Small" },
    { 1, "Normal" },
    { 2, "Large" },
    { 3, "X-Large" },
};

static const std::unordered_map<int32_t, const char*> menuThemeOptions = {
    { UIWidgets::Colors::Red, "Red" },
    { UIWidgets::Colors::DarkRed, "Dark Red" },
    { UIWidgets::Colors::Orange, "Orange" },
    { UIWidgets::Colors::Green, "Green" },
    { UIWidgets::Colors::DarkGreen, "Dark Green" },
    { UIWidgets::Colors::LightBlue, "Light Blue" },
    { UIWidgets::Colors::Blue, "Blue" },
    { UIWidgets::Colors::DarkBlue, "Dark Blue" },
    { UIWidgets::Colors::Indigo, "Indigo" },
    { UIWidgets::Colors::Violet, "Violet" },
    { UIWidgets::Colors::Purple, "Purple" },
    { UIWidgets::Colors::Brown, "Brown" },
    { UIWidgets::Colors::Gray, "Gray" },
    { UIWidgets::Colors::DarkGray, "Dark Gray" },
};

static const std::unordered_map<int32_t, const char*> textureFilteringMap = {
    { Fast::FILTER_THREE_POINT, "Three-Point" },
    { Fast::FILTER_LINEAR, "Linear" },
    { Fast::FILTER_NONE, "None" },
};

static const std::unordered_map<int32_t, const char*> notificationPosition = {
    { 0, "Top Left" }, { 1, "Top Right" }, { 2, "Bottom Left" }, { 3, "Bottom Right" }, { 4, "Hidden" },
};

#ifdef __SWITCH__
static const std::unordered_map<int32_t, const char*> switchPerformanceProfiles = {
    { Ship::MAXIMUM, SWITCH_CPU_PROFILES[Ship::MAXIMUM] },
    { Ship::HIGH, SWITCH_CPU_PROFILES[Ship::HIGH] },
    { Ship::BOOST, SWITCH_CPU_PROFILES[Ship::BOOST] },
    { Ship::STOCK, SWITCH_CPU_PROFILES[Ship::STOCK] },
    { Ship::POWERSAVINGM1, SWITCH_CPU_PROFILES[Ship::POWERSAVINGM1] },
    { Ship::POWERSAVINGM2, SWITCH_CPU_PROFILES[Ship::POWERSAVINGM2] },
    { Ship::POWERSAVINGM3, SWITCH_CPU_PROFILES[Ship::POWERSAVINGM3] }
};
#endif

void GhostshipMenu::AddMenuSettings() {
    // Add Settings Menu
    AddMenuEntry("Settings", CVAR_SETTING("Menu.SettingsSidebarSection"));
    AddSidebarEntry("Settings", "General", 2);
    WidgetPath path = { "Settings", "General", SECTION_COLUMN_1 };

    // General - Settings
    AddWidget(path, "Menu Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Menu Theme", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_SETTING("Menu.Theme"))
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .Tooltip("Changes the Theme of the Menu Widgets.")
                     .ComboMap(menuThemeOptions)
                     .DefaultIndex(Colors::LightBlue));
    AddWidget(path, "Menu Controller Navigation", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_IMGUI_CONTROLLER_NAV)
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Allows controller navigation of the port menu (Settings, Enhancements,...)\nCAUTION: "
                              "This will disable game inputs while the menu is visible.\n\nD-pad to move between "
                              "items, A to select, B to move up in scope.")
#ifdef __SWITCH__
                     .DefaultValue(true)
#endif
        );
    AddWidget(path, "Menu Background Opacity", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_SETTING("Menu.BackgroundOpacity"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().DefaultValue(0.85f).IsPercentage().Tooltip(
            "Sets the opacity of the background of the port menu."));

    AddWidget(path, "General Settings", WIDGET_SEPARATOR_TEXT);
#if not defined(__SWITCH__) and not defined(__WIIU__)
    AddWidget(path, "Cursor Always Visible", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("CursorVisibility"))
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            ShipCompat::GetWindow()->SetForceCursorVisibility(CVarGetInteger(CVAR_SETTING("CursorVisibility"), 0));
        })
        .Options(CheckboxOptions().Tooltip("Makes the cursor always visible, even in full screen."));
#endif
    AddWidget(path, "Search In Sidebar", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.SidebarSearch"))
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            if (CVarGetInteger(CVAR_SETTING("Menu.SidebarSearch"), 0)) {
                mGhostshipMenu->InsertSidebarSearch();
            } else {
                mGhostshipMenu->RemoveSidebarSearch();
            }
        })
        .Options(CheckboxOptions().Tooltip(
            "Displays the Search menu as a sidebar entry in Settings instead of in the header."));
    AddWidget(path, "Search Input Autofocus", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.SearchAutofocus"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Search input box gets autofocus when visible. Does not affect using other widgets."));
#ifdef __SWITCH__
    AddWidget(path, "Hardware", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Switch performance mode", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_SWITCH_PERF_MODE)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) { Ship::Switch::ApplyOverclock(); })
        .Options(ComboboxOptions()
                     .DefaultIndex(Ship::MAXIMUM)
                     .ComboMap(switchPerformanceProfiles)
                     .Tooltip("Sets the Nintendo Switch CPU performance profile."));
#endif
#if not defined(__SWITCH__) and not defined(__WIIU__)
    AddWidget(path, "Open App Files Folder", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            std::string filesPath = Ship::Context::GetAppDirectoryPath();
            SDL_OpenURL(std::string("file:///" + std::filesystem::absolute(filesPath).string()).c_str());
        })
        .Options(ButtonOptions().Tooltip("Opens the folder that contains the save and mods folders, etc."));
#endif
    AddWidget(path, "EXPERIMENTAL", WIDGET_SEPARATOR_TEXT).Options(TextOptions().Color(Colors::Orange));
    AddWidget(path, "ImGui Menu Scaling", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_SETTING("ImGuiScale"))
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .ComboMap(imguiScaleOptions)
                     .Tooltip("Changes the scaling of the ImGui menu elements.")
                     .DefaultIndex(1)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Far))
        .Callback([](WidgetInfo& info) { GameEngine::Instance->ScaleImGui(); });

    // General - About
    path.column = SECTION_COLUMN_2;

    AddWidget(path, "About", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Ghostship", WIDGET_TEXT);
    if (gGitCommitTag[0] != 0) {
        AddWidget(path, gBuildVersion, WIDGET_TEXT);
    } else {
        AddWidget(path, ("Branch: " + std::string(gGitBranch)), WIDGET_TEXT);
        AddWidget(path, ("Commit: " + std::string(gGitCommitHash)), WIDGET_TEXT);
    }
    // for (uint32_t i = 0; i < ResourceMgr_GetNumGameVersions(); i++) {
    //     AddWidget(path, GetGameVersionString(i), WIDGET_TEXT);
    // }

    // Audio Settings
    path.sidebarName = "Audio";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Audio", 3);

    AddWidget(path, "Master Volume: %d %%", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_SETTING("Volume.Master"))
        .RaceDisable(false)
        .Options(IntSliderOptions().Min(0).Max(100).DefaultValue(50).ShowButtons(true).Format(""));
    AddWidget(path, "Main Music Volume: %d %%", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_SETTING("Volume.MainMusic"))
        .RaceDisable(false)
        .Options(IntSliderOptions().Min(0).Max(100).DefaultValue(100).ShowButtons(true).Format(""))
        .Callback([](WidgetInfo& info) {
            audio_set_player_volume(SEQ_PLAYER_LEVEL,
                                    ((float)CVarGetInteger(CVAR_SETTING("Volume.MainMusic"), 100) / 100.0f));
        });
    AddWidget(path, "Environment Volume: %d %%", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_SETTING("Volume.Environment"))
        .RaceDisable(false)
        .Options(IntSliderOptions().Min(0).Max(100).DefaultValue(100).ShowButtons(true).Format(""))
        .Callback([](WidgetInfo& info) {
            audio_set_player_volume(SEQ_PLAYER_ENV,
                                    ((float)CVarGetInteger(CVAR_SETTING("Volume.Environment"), 100) / 100.0f));
        });
    AddWidget(path, "Sound Effects Volume: %d %%", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_SETTING("Volume.SFX"))
        .RaceDisable(false)
        .Options(IntSliderOptions().Min(0).Max(100).DefaultValue(100).ShowButtons(true).Format(""))
        .Callback([](WidgetInfo& info) {
            audio_set_player_volume(SEQ_PLAYER_SFX, ((float)CVarGetInteger(CVAR_SETTING("Volume.SFX"), 100) / 100.0f));
        });
    AddWidget(path, "Audio API (Needs reload)", WIDGET_AUDIO_BACKEND).RaceDisable(false);

    // Graphics Settings
    static int32_t maxFps = 360;
    const char* tooltip = "Uses Matrix Interpolation to create extra frames, resulting in smoother graphics. This is "
                          "purely visual and does not impact game logic, execution of glitches etc.\n\nA higher target "
                          "FPS than your monitor's refresh rate will waste resources, and might give a worse result.";
    path.sidebarName = "Graphics";
    AddSidebarEntry("Settings", "Graphics", 3);
    AddWidget(path, "Graphics Options", WIDGET_SEPARATOR_TEXT);
#ifndef __SWITCH__
    AddWidget(path, "Toggle Fullscreen", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) { ShipCompat::GetWindow()->ToggleFullscreen(); })
        .Options(ButtonOptions().Tooltip("Toggles Fullscreen On/Off."));
#endif
    AddWidget(path, "Internal Resolution", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_INTERNAL_RESOLUTION)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            ShipCompat::GetWindow()->SetResolutionMultiplier(CVarGetFloat(CVAR_INTERNAL_RESOLUTION, 1));
        })
        .PreFunc([](WidgetInfo& info) {
            if (mGhostshipMenu->disabledMap.at(DISABLE_FOR_ADVANCED_RESOLUTION_ON).active &&
                mGhostshipMenu->disabledMap.at(DISABLE_FOR_VERTICAL_RES_TOGGLE_ON).active) {
                info.activeDisables.push_back(DISABLE_FOR_ADVANCED_RESOLUTION_ON);
                info.activeDisables.push_back(DISABLE_FOR_VERTICAL_RES_TOGGLE_ON);
            } else if (mGhostshipMenu->disabledMap.at(DISABLE_FOR_LOW_RES_MODE_ON).active) {
                info.activeDisables.push_back(DISABLE_FOR_LOW_RES_MODE_ON);
            }
        })
        .Options(
            FloatSliderOptions()
                .Tooltip("Multiplies your output resolution by the value inputted, as a more intensive but effective "
                         "form of anti-aliasing.")
                .ShowButtons(false)
                .IsPercentage()
                .Min(0.5f)
                .Max(2.0f));
#ifndef __WIIU__
    AddWidget(path, "Anti-aliasing (MSAA)", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_MSAA_VALUE)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) { ShipCompat::GetWindow()->SetMsaaLevel(CVarGetInteger(CVAR_MSAA_VALUE, 1)); })
        .Options(
            IntSliderOptions()
                .Tooltip("Activates MSAA (multi-sample anti-aliasing) from 2x up to 8x, to smooth the edges of "
                         "rendered geometry.\n"
                         "Higher sample count will result in smoother edges on models, but may reduce performance.")
                .Min(1)
                .Max(8)
                .DefaultValue(1));
#endif
    auto fps = CVarGetInteger(CVAR_SETTING("InterpolationFPS"), 30);
    const char* fpsFormat = fps == 30 ? "Original (%d)" : "%d";
    AddWidget(path, "Current FPS", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_SETTING("InterpolationFPS"))
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            auto options = std::static_pointer_cast<IntSliderOptions>(info.options);
            int32_t defaultValue = options->defaultValue;
            if (CVarGetInteger(info.cVar, defaultValue) == defaultValue) {
                options->format = "Original (%d)";
            } else {
                options->format = "%d";
            }
        })
        .PreFunc([](WidgetInfo& info) {
            if (mGhostshipMenu->disabledMap.at(DISABLE_FOR_MATCH_REFRESH_RATE_ON).active)
                info.activeDisables.push_back(DISABLE_FOR_MATCH_REFRESH_RATE_ON);
        })
        .Options(IntSliderOptions().Tooltip(tooltip).Min(30).Max(maxFps).DefaultValue(30).Format(fpsFormat));
    AddWidget(path, "Match Refresh Rate", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("MatchRefreshRate"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Matches interpolation value to the refresh rate of your display."));
    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND).RaceDisable(false);
    AddWidget(path, "Enable Vsync", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_VSYNC_ENABLED)
        .RaceDisable(false)
        .PreFunc([](WidgetInfo& info) { info.isHidden = mGhostshipMenu->disabledMap.at(DISABLE_FOR_NO_VSYNC).active; })
        .Options(CheckboxOptions()
                     .Tooltip("Removes tearing, but clamps your max FPS to your displays refresh rate.")
                     .DefaultValue(true));
    AddWidget(path, "Windowed Fullscreen", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SDL_WINDOWED_FULLSCREEN)
        .RaceDisable(false)
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mGhostshipMenu->disabledMap.at(DISABLE_FOR_NO_WINDOWED_FULLSCREEN).active;
        })
        .Options(CheckboxOptions().Tooltip("Enables Windowed Fullscreen Mode."));
    AddWidget(path, "Allow multi-windows", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENABLE_MULTI_VIEWPORTS)
        .RaceDisable(false)
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mGhostshipMenu->disabledMap.at(DISABLE_FOR_NO_MULTI_VIEWPORT).active;
        })
        .Options(CheckboxOptions()
                     .Tooltip("Allows multiple windows to be opened at once. Requires a reload to take effect.")
                     .DefaultValue(true));
    AddWidget(path, "Texture Filter (Needs reload)", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_TEXTURE_FILTER)
        .RaceDisable(false)
        .Options(ComboboxOptions().Tooltip("Sets the applied Texture Filtering.").ComboMap(textureFilteringMap));

    path.column = SECTION_COLUMN_2;
    AddWidget(path, "Advanced Graphics Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "HD Texture Upload Budget", WIDGET_CVAR_SLIDER_INT)
        .CVar("gEnhancements.Graphics.TextureUploadBudget")
        .RaceDisable(false)
        .Options(IntSliderOptions()
                     .Tooltip("Max new HD-pack texture uploads per frame (0 = unlimited). Lower values spread big "
                              "4K uploads across frames so entering a new area doesn't hitch; the base texture "
                              "shows until each HD replacement is ready.")
                     .Min(0)
                     .Max(8)
                     .DefaultValue(1));
    AddWidget(path, "Async Texture Loading", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Graphics.AsyncTextureLoad")
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Decodes HD/replacement textures on a background thread instead of blocking the "
                              "render thread, so loading a level doesn't hitch. The vanilla texture shows until "
                              "its HD version finishes loading, then swaps in. Only active with Alternative "
                              "Assets enabled.")
                     .DefaultValue(false));
    AddWidget(path, "Debug HD Replacement", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Graphics.TextureReplacementDebug")
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Tints draws by HD-replacement state: blue = HD active, green flash = just "
                              "uploaded, red = base shown while the HD upload is still pending.")
                     .DefaultValue(false));

    path.sidebarName = "Shaders";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Shaders", 1);

    AddWidget(path, "Shader Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Shader Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ShaderSettings"))
        .RaceDisable(false)
        .WindowName("Shader Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Tweakables for shader packs: post-processing passes and @setting "
                                               "values declared by custom shaders."));

    // Controls
    path.sidebarName = "Controls";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Controls", 1);
    AddWidget(path, "Clear Devices", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) {
            GhostshipGui::mModalWindow->RegisterPopup(
                "Clear Config",
                "This will completely erase the controls config, including registered devices.\nContinue?", "Clear",
                "Cancel",
                []() {
                    ShipCompat::GetConsoleVariables()->ClearBlock(CVAR_PREFIX_SETTING ".Controllers");
                    uint8_t bits = 0;
                    ShipCompat::GetControlDeck()->Init(&bits);
                },
                nullptr);
        })
        .Options(ButtonOptions().Size(Sizes::Inline));
    AddWidget(path, "Controller Bindings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Bindings Window", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ControllerConfiguration"))
        .RaceDisable(false)
        .WindowName("Configure Controller")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Bindings Window."));

    AddWidget(path, "Touch Controls", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Enable Touch Controls", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_TOUCH("Enabled"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Shows an on-screen virtual controller for touch screens.\nOn desktop the mouse can drive it for "
            "testing."));
    AddWidget(path, "Touch Controls Scale", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_TOUCH("Scale"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(0.5f).Max(2.0f).DefaultValue(1.0f).Tooltip(
            "Size of the on-screen buttons and stick."));
    AddWidget(path, "Touch Controls Opacity", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_TOUCH("Opacity"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(0.1f).Max(1.0f).DefaultValue(0.7f).IsPercentage().Tooltip(
            "Opacity of the on-screen controls."));
    AddWidget(path, "Edit Touch Layout", WIDGET_BUTTON)
        .Options(ButtonOptions().Tooltip(
            "Closes the menu and lets you drag the on-screen controls to new positions.\nTap Done to save or Reset "
            "to restore the default layout."))
        .Callback([](WidgetInfo& info) {
            CVarSetInteger(CVAR_TOUCH("Enabled"), 1);
            CVarSetInteger(CVAR_TOUCH("EditMode"), 1);
            ShipCompat::GetWindow()->GetGui()->GetMenu()->Hide();
        });

    // Input Viewer
    path.sidebarName = "Input Viewer";
    AddSidebarEntry("Settings", path.sidebarName, 3);
    AddWidget(path, "Input Viewer", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Input Viewer", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("InputViewer"))
        .RaceDisable(false)
        .WindowName("Input Viewer")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Input Viewer.").EmbedWindow(false));

    AddWidget(path, "Input Viewer Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Input Viewer Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("InputViewerSettings"))
        .RaceDisable(false)
        .WindowName("Input Viewer Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Input Viewer Settings Window."));

    // Notifications
    path.sidebarName = "Notifications";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", path.sidebarName, 3);
    AddWidget(path, "Position", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_SETTING("Notifications.Position"))
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .Tooltip("Which corner of the screen notifications appear in.")
                     .ComboMap(notificationPosition)
                     .DefaultIndex(3));
    AddWidget(path, "Duration (seconds):", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_SETTING("Notifications.Duration"))
        .RaceDisable(false)
        .Options(FloatSliderOptions()
                     .Tooltip("How long notifications are displayed for.")
                     .Format("%.1f")
                     .Step(0.1f)
                     .Min(3.0f)
                     .Max(30.0f)
                     .DefaultValue(10.0f));
    AddWidget(path, "Background Opacity", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_SETTING("Notifications.BgOpacity"))
        .RaceDisable(false)
        .Options(FloatSliderOptions()
                     .Tooltip("How opaque the background of notifications is.")
                     .DefaultValue(0.5f)
                     .IsPercentage());
    AddWidget(path, "Size:", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_SETTING("Notifications.Size"))
        .RaceDisable(false)
        .Options(FloatSliderOptions()
                     .Tooltip("How large notifications are.")
                     .Format("%.1f")
                     .Step(0.1f)
                     .Min(1.0f)
                     .Max(5.0f)
                     .DefaultValue(1.8f));
    AddWidget(path, "Test Notification", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) {
            Notification::Emit({
                .prefix = "Welcome",
                .message = "to",
                .suffix = "Ghostship.",
            });
        })
        .Options(ButtonOptions().Tooltip("Displays a test notification."));
    AddWidget(path, "Test Achievement Notification", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo& info) { Notification::EmitAchievement(nullptr, "Harbour Mastery", 0); })
        .Options(ButtonOptions().Tooltip("Displays a test notification."));
}

} // namespace GhostshipGui
