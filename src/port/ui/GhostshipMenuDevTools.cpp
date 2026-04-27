#include "GhostshipMenu.h"
#include "port/mods/BetterLevelSelect.h"
#include "game/object_list_processor.h"
#include "include/behavior_data.h"
#include "game/level_update.h"
#include "port/Engine.h"
#include "ship/utils/StringHelper.h"
#include "ship/scripting/ScriptLoader.h"

extern "C" {
struct Object* spawn_object_abs_with_rot(struct Object* parent, s16 uselessArg, u32 model,
                                         const BehaviorScript* behavior, s16 x, s16 y, s16 z, s16 pitch, s16 yaw,
                                         s16 roll);
}

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
using namespace UIWidgets;

static const std::unordered_map<int32_t, const char*> logLevels = {
    { DEBUG_LOG_TRACE, "Trace" }, { DEBUG_LOG_DEBUG, "Debug" }, { DEBUG_LOG_INFO, "Info" },
    { DEBUG_LOG_WARN, "Warn" },   { DEBUG_LOG_ERROR, "Error" }, { DEBUG_LOG_CRITICAL, "Critical" },
    { DEBUG_LOG_OFF, "Off" },
};

static const std::unordered_map<int32_t, const char*> debugInfoPages = {
    { DEBUG_PAGE_OBJECTINFO, "Object" }, { DEBUG_PAGE_CHECKSURFACEINFO, "Check Surface" },
    { DEBUG_PAGE_MAPINFO, "Map" },       { DEBUG_PAGE_STAGEINFO, "Stage" },
    { DEBUG_PAGE_EFFECTINFO, "Effect" }, { DEBUG_PAGE_ENEMYINFO, "Enemy" },
};

static const std::unordered_map<int32_t, const char*> language = {
    { 0, "English" },
    { 1, "Japanese" },
};

#ifdef _DEBUG
DebugLogOption defaultLogLevel = DEBUG_LOG_DEBUG;
#else
DebugLogOption defaultLogLevel = DEBUG_LOG_INFO;
#endif

void GhostshipMenu::AddMenuDevTools() {
    // Add Dev Tools Menu
    AddMenuEntry("Dev Tools", CVAR_SETTING("Menu.DevToolsSidebarSection"));

    // General
    AddSidebarEntry("Dev Tools", "General", 3);
    WidgetPath path = { "Dev Tools", "General", SECTION_COLUMN_1 };

    AddWidget(path, "Popout Menu", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.Popout"))
        .Options(CheckboxOptions().Tooltip("Changes the menu display from overlay to windowed."));
    AddWidget(path, "Log Level", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("LogLevel"))
        .Options(ComboboxOptions()
                     .Tooltip("The log level determines which messages are printed to the console."
                              " This does not affect the log file output")
                     .ComboMap(logLevels)
                     .DefaultIndex(defaultLogLevel))
        .Callback([](WidgetInfo& info) {
            Ship::Context::GetInstance()->GetLogger()->set_level(
                (spdlog::level::level_enum)CVarGetInteger(CVAR_DEVELOPER_TOOLS("LogLevel"), defaultLogLevel));
        })
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mGhostshipMenu->disabledMap.at(DISABLE_FOR_DEBUG_MODE_OFF).active;
        });
#ifdef USE_GBI_TRACE
    AddWidget(path, "GFX Trace Mode", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("GFXTrace"))
        .Options(CheckboxOptions().Tooltip(
            "Enables the Gfx trace mode, which will output information about the Gfx commands being run."));
#endif
    AddWidget(path, "Debug Mode", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DebugMode"))
        .Options(CheckboxOptions().Tooltip("Various debug features, including a level selector from the main menu."));
    AddWidget(path, "Better Level Select", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("BetterLevelSelect"))
        .Options(CheckboxOptions().Tooltip(
            "Tweaks to the level select screen, like naming and allowing C-buttons to be used."))
        .Callback([](WidgetInfo& info) { BetterLevelSelect_HandleReload(); })
        .PreFunc(
            [](WidgetInfo& info) { info.options->Disabled(!CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugMode"), 0)); });
    AddWidget(path, "Level Select Language", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("BLSLanguage"))
        .Options(ComboboxOptions().Tooltip("Language used in Better Level Select").ComboMap(language))
        .PreFunc([](WidgetInfo& info) {
            info.options->Disabled(!CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugMode"), 0) ||
                                   !CVarGetInteger(CVAR_DEVELOPER_TOOLS("BetterLevelSelect"), 0));
        });
    AddWidget(path, "Draw Debug Info", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DrawDebugInfo"))
        .Options(CheckboxOptions().Tooltip("Draws Debug Related Information"));
    AddWidget(path, "Debug Info Mode", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DebugInfoPage"))
        .Options(
            ComboboxOptions().Tooltip("Select Debug Page").ComboMap(debugInfoPages).DefaultIndex(DEBUG_PAGE_OBJECTINFO))
        .PreFunc([](WidgetInfo& info) {
            info.options->Disabled(!CVarGetInteger(CVAR_DEVELOPER_TOOLS("DrawDebugInfo"), 0));
        });

    // Save Editor
    path.sidebarName = "Save Editor";
    AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    AddWidget(path, "Popout Save Editor", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("SaveEditor"))
        .WindowName("Save Editor")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Save Editor Window."));

    // Console
    path.sidebarName = "Console";
    AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    AddWidget(path, "Popout Console", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("DevConsole"))
        .WindowName("Console##Dev")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Console Window."));

    path.sidebarName = "Event Debugger";
    AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    AddWidget(path, "Popout Event Debugger", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("EventDebugger"))
        .WindowName("Event Debugger")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Event Debugger Window."));

    path.sidebarName = "Object Viewer";
    AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    AddWidget(path, "Popout Object Viewer", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ObjectViewer"))
        .WindowName("Object Viewer##Dev")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Object Viewer Window."));
}

void GhostshipMenu::AddModMenu() {
#ifndef DISABLE_SCRIPTING
    auto mods = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetArchives();
    AddMenuEntry("Mods", CVAR_SETTING("Menu.ModsSidebarSection"));

    WidgetPath path = { "Mods", "General", SECTION_COLUMN_1 };

    AddSidebarEntry(path.sectionName, path.sidebarName, 1);
    AddWidget(path, "Reload Scripts", WIDGET_BUTTON)
        .Options(ButtonOptions().Tooltip("Reloads all scripts from disk.").Color(Colors::Orange))
        .Callback([](WidgetInfo& info) {
            Ship::Context::GetInstance()->GetScriptLoader()->UnloadAll();
            GameEngine::Instance->LoadScripts();
        });

    auto keystore = Ship::Context::GetInstance()->GetKeystore();
    auto allKeys = keystore->GetAllKeys();

    for (const auto& entry : *mods) {
        const auto& info = entry->GetManifest();
        if (info.Name.empty()) {
            continue;
        }

        std::string cardTitle = info.Name;

        if (!info.Icon.empty()) {
            cardTitle = info.Icon + " " + cardTitle;
        }

        if (!info.Main.empty() || !info.Binaries.empty()) {
            cardTitle += " (Code Mod)";
        }

        AddWidget(path, cardTitle, WIDGET_SEPARATOR_TEXT).Options(UIWidgets::TextOptions{});

        std::string metadata = "Author: " + (info.Author.empty() ? "Unknown" : info.Author);

        if (!info.Version.empty()) {
            metadata += "  |  Version: " + info.Version;
        }
        if (!info.License.empty()) {
            metadata += "  |  License: " + info.License;
        }

        AddWidget(path, metadata, WIDGET_TEXT).Options(UIWidgets::TextOptions{});
        Ship::KeyOrigin origin = Ship::KeyOrigin::User;
        for (const auto& key : allKeys) {
            if (key.Data == StringHelper::HexToBytes(info.PublicKey)) {
                origin = key.Origin;
                break;
            }
        }

        std::string securityText;
        if (entry->IsSigned()) {
            securityText = std::string(ICON_FA_CHECK_CIRCLE) + " Security: Signed (Trusted)";
            std::string originText;
            Colors color = Colors::Green;
            switch (origin) {
                case Ship::KeyOrigin::User:
                    originText = "[User Approved]";
                    color = Colors::Yellow;
                    break;
                case Ship::KeyOrigin::Game:
                    originText = "[Game]";
                    color = Colors::Purple;
                    break;
                case Ship::KeyOrigin::System:
                    originText = "[System]";
                    color = Colors::Red;
                    break;
            }

            AddWidget(path, securityText, WIDGET_TEXT).Options(UIWidgets::TextOptions{ .color = Colors::Green });
            AddWidget(path, originText, WIDGET_TEXT).SameLine(true).Options(UIWidgets::TextOptions{ .color = color });
        } else if (entry->IsChecksumValid()) {
            securityText = std::string(ICON_FA_EXCLAMATION_TRIANGLE) + " Security: Unsigned (Caution)";
            AddWidget(path, securityText, WIDGET_TEXT).Options(UIWidgets::TextOptions{ .color = Colors::Orange });
        } else {
            securityText = std::string(ICON_FA_EXCLAMATION_TRIANGLE) + " Security: Untrusted";
            AddWidget(path, securityText, WIDGET_TEXT).Options(UIWidgets::TextOptions{ .color = Colors::Red });
        }

        if (!info.Dependencies.empty()) {
            std::string depsString = "Dependencies: ";
            for (size_t i = 0; i < info.Dependencies.size(); ++i) {
                depsString += info.Dependencies[i];
                if (i < info.Dependencies.size() - 1)
                    depsString += ", ";
            }

            AddWidget(path, depsString, WIDGET_TEXT).Options(UIWidgets::TextOptions{});
        }

        if (!info.Description.empty()) {
            AddWidget(path, info.Description, WIDGET_TEXT).Options(UIWidgets::TextOptions{});
        }

        if (!info.Website.empty()) {
            AddWidget(path, "Open Webpage##" + info.Name, WIDGET_BUTTON)
                .Options(UIWidgets::ButtonOptions{})
                .Callback([info](WidgetInfo&) { SDL_OpenURL(info.Website.c_str()); });
        }

        AddWidget(path, "##Spacer_" + info.Name, WIDGET_SEPARATOR).Options(UIWidgets::WidgetOptions{});
    }
#endif
};

} // namespace GhostshipGui
