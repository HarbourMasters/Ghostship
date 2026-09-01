#include "GhostshipMenu.h"
#include "port/ShipCompat.h"
#include "GhostshipInputEditorWindow.h"
#include <ship/window/gui/GuiMenuBar.h>
#include <ship/window/gui/GuiElement.h>
#include <variant>
#include <ship/utils/StringHelper.h>
#include <spdlog/fmt/fmt.h>
#include <tuple>

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

static const std::vector<const char*> textureFilteringOptions = {
    "Three-Point", // Fast::FILTER_THREE_POINT,
    "Linear",      // Fast::FILTER_LINEAR
    "None",        // Fast::FILTER_NONE
};

#ifdef _DEBUG
DebugLogOption defaultLogLevel = DEBUG_LOG_TRACE;
#else
DebugLogOption defaultLogLevel = DEBUG_LOG_INFO;
#endif

static const std::vector<const char*> logLevels = {
    "Trace",    // DEBUG_LOG_TRACE
    "Debug",    // DEBUG_LOG_DEBUG
    "Info",     // DEBUG_LOG_INFO
    "Warn",     // DEBUG_LOG_WARN
    "Error",    // DEBUG_LOG_ERROR
    "Critical", // DEBUG_LOG_CRITICAL
    "Off",      // DEBUG_LOG_OFF
};

extern std::unordered_map<s16, const char*> warpPointSceneList;

namespace GhostshipGui {
extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;

using namespace UIWidgets;

void GhostshipMenu::AddSidebarEntry(std::string sectionName, std::string sidebarName, uint32_t columnCount) {
    assert(!sectionName.empty());
    assert(!sidebarName.empty());
    menuEntries.at(sectionName).sidebars.emplace(sidebarName, SidebarEntry{ .columnCount = columnCount });
    menuEntries.at(sectionName).sidebarOrder.push_back(sidebarName);
}

void GhostshipMenu::RemoveMenuEntry(std::string entryName) {
    assert(!entryName.empty());
    if (menuEntries.contains(entryName)) {
        menuEntries.at(entryName).markForDelete = true;
    }
}

void GhostshipMenu::RemoveSidebarEntry(std::string sectionName, std::string sidebarName) {
    assert(!sectionName.empty());
    assert(!sidebarName.empty());
    auto& sidebars = menuEntries.at(sectionName).sidebars;
    // Delete directly
    if (sidebars.contains(sidebarName)) {
        sidebars.erase(sidebarName);
        auto& sidebarOrder = menuEntries.at(sectionName).sidebarOrder;
        sidebarOrder.erase(std::remove(sidebarOrder.begin(), sidebarOrder.end(), sidebarName), sidebarOrder.end());
    } else {
        // Mark for deletion if not found, will be deleted when encountered in Draw()
        menuEntries.at(sectionName).sidebars.at(sidebarName).markForDelete = true;
    }
}

WidgetInfo& GhostshipMenu::AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType) {
    assert(!widgetName.empty());                        // Must be unique
    assert(menuEntries.contains(pathInfo.sectionName)); // Section/header must already exist
    assert(menuEntries.at(pathInfo.sectionName).sidebars.contains(pathInfo.sidebarName)); // Sidebar must already exist
    std::unordered_map<std::string, SidebarEntry>& sidebar = menuEntries.at(pathInfo.sectionName).sidebars;
    uint8_t column = pathInfo.column;
    if (sidebar.contains(pathInfo.sidebarName)) {
        while (sidebar.at(pathInfo.sidebarName).columnWidgets.size() < column + 1) {
            sidebar.at(pathInfo.sidebarName).columnWidgets.push_back({});
        }
    }
    SidebarEntry& entry = sidebar.at(pathInfo.sidebarName);
    entry.columnWidgets.at(column).push_back({ .name = widgetName, .type = widgetType });
    WidgetInfo& widget = entry.columnWidgets.at(column).back();
    switch (widgetType) {
        case WIDGET_CHECKBOX:
        case WIDGET_CVAR_CHECKBOX:
            widget.options = std::make_shared<CheckboxOptions>();
            break;
        case WIDGET_SLIDER_FLOAT:
        case WIDGET_CVAR_SLIDER_FLOAT:
            widget.options = std::make_shared<FloatSliderOptions>();
            break;
        case WIDGET_SLIDER_INT:
        case WIDGET_CVAR_SLIDER_INT:
            widget.options = std::make_shared<IntSliderOptions>();
            break;
        case WIDGET_COMBOBOX:
        case WIDGET_CVAR_COMBOBOX:
        case WIDGET_AUDIO_BACKEND:
        case WIDGET_VIDEO_BACKEND:
            widget.options = std::make_shared<ComboboxOptions>();
            break;
        case WIDGET_BUTTON:
            widget.options = std::make_shared<ButtonOptions>();
            break;
        case WIDGET_WINDOW_BUTTON:
            widget.options = std::make_shared<WindowButtonOptions>();
            break;
        case WIDGET_CVAR_COLOR_PICKER:
        case WIDGET_COLOR_PICKER:
            widget.options = std::make_shared<ColorPickerOptions>();
            break;
        case WIDGET_SEPARATOR_TEXT:
        case WIDGET_TEXT:
            widget.options = std::make_shared<TextOptions>();
            break;
        case WIDGET_SEARCH:
        case WIDGET_SEPARATOR:
        default:
            widget.options = std::make_shared<WidgetOptions>();
    }
    return widget;
}

GhostshipMenu::GhostshipMenu(const std::string& consoleVariable, const std::string& name)
    : Menu(consoleVariable, name, 0, UIWidgets::Colors::LightBlue) {
}

void GhostshipMenu::OnInit(const nlohmann::json& initArgs) {
    Ship::Menu::OnInit(initArgs);
    AddMenuSettings();
    AddMenuEnhancements();
    AddMenuRando();
    AddMenuAchievements();
    AddMenuDevTools();
#ifdef ENABLE_SCRIPTING
    AddModMenu();
#endif

    if (CVarGetInteger(CVAR_SETTING("Menu.SidebarSearch"), 0)) {
        InsertSidebarSearch();
    }

    for (auto& initFunc : MenuInit::GetInitFuncs()) {
        initFunc();
    }

    disabledMap = {
        { DISABLE_FOR_NO_VSYNC,
          { [](disabledInfo& info) -> bool { return !ShipCompat::GetWindow()->CanDisableVerticalSync(); },
            "Disabling VSync not supported" } },
        { DISABLE_FOR_NO_WINDOWED_FULLSCREEN,
          { [](disabledInfo& info) -> bool { return !ShipCompat::GetWindow()->SupportsWindowedFullscreen(); },
            "Windowed Fullscreen not supported" } },
        { DISABLE_FOR_NO_MULTI_VIEWPORT,
          { [](disabledInfo& info) -> bool { return !ShipCompat::GetWindow()->GetGui()->SupportsViewports(); },
            "Multi-viewports not supported" } },
        { DISABLE_FOR_NOT_DIRECTX,
          { [](disabledInfo& info) -> bool {
               return ShipCompat::GetWindow()->GetWindowBackend() != Fast::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Available Only on DirectX" } },
        { DISABLE_FOR_DIRECTX,
          { [](disabledInfo& info) -> bool {
               return ShipCompat::GetWindow()->GetWindowBackend() == Fast::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Not Available on DirectX" } },
        { DISABLE_FOR_MATCH_REFRESH_RATE_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0); },
            "Match Refresh Rate is Enabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution Enabled" } },
        { DISABLE_FOR_VERTICAL_RES_TOGGLE_ON,
          { [](disabledInfo& info) -> bool {
               return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle Enabled" } },
        { DISABLE_FOR_LOW_RES_MODE_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_LOW_RES_MODE, 0); }, "N64 Mode Enabled" } },
        { DISABLE_FOR_NULL_PLAY_STATE,
          { [](disabledInfo& info) -> bool { return /*gPlayState == NULL*/ "TODO"; }, "Save Not Loaded" } },
        { DISABLE_FOR_DEBUG_MODE_OFF,
          { [](disabledInfo& info) -> bool { return !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); },
            "Debug Mode is Disabled" } },
        { DISABLE_FOR_FRAME_ADVANCE_OFF,
          { [](disabledInfo& info) -> bool {
               return /*!(gPlayState != nullptr && gPlayState->frameAdvCtx.enabled)*/ "TODO";
           },
            "Frame Advance is Disabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_OFF,
          { [](disabledInfo& info) -> bool { return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution is Disabled" } },
        { DISABLE_FOR_VERTICAL_RESOLUTION_OFF,
          { [](disabledInfo& info) -> bool {
               return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle is Off" } },
        { DISABLE_FOR_BOOT_TO_DEBUG_WARP_SCREEN_ON,
          { [](disabledInfo& info) -> bool {
               return CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0) &&
                      CVarGetInteger(CVAR_DEVELOPER_TOOLS("BootToDebugWarpScreen"), 0);
           },
            "\"Boot To Debug Warp Screen\" Enabled (see Dev Tools -> General)" } },
    };
}

void GhostshipMenu::UpdateElement() {
    Ship::Menu::UpdateElement();
}

void GhostshipMenu::Draw() {
    Ship::Menu::Draw();
}

void GhostshipMenu::DrawElement() {
    Ship::Menu::DrawElement();
}
} // namespace GhostshipGui
