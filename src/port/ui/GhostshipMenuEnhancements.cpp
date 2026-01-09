#include "GhostshipMenu.h"

#define CVAR_INT_SHIP_INIT(cvar, val) \
    CVarSetInteger(cvar, val);        \
    ShipInit::Init(cvar);

static std::string comboboxTooltip = "";

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
using namespace UIWidgets;

void GhostshipMenu::AddMenuEnhancements() {
    // Add Enhancements Menu
    AddMenuEntry("Enhancements", CVAR_SETTING("Menu.EnhancementsSidebarSection"));

    // Quality of Life
    WidgetPath path = { "Enhancements", "Gameplay", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Disable LoD", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("DisableLOD"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Disable Level of Detail (LOD) to avoid models using "
                                           "lower poly versions at a distance"));
    AddWidget(path, "Select Any Star", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("SelectAllStars"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Let's you select any star from the menu regardless "
                                           "of the courses completion status."));
    AddWidget(path, "Collecting Stars Will Not Exit Level", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("StarNoExit"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Collecting Stars will not take you out of the level."));
    AddWidget(path, "Skip Intro Peach Cutscene", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("DisablePeachCutscene"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Skips the Peach cutscene when starting a new game."));

    path = { "Enhancements", "Fixes", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Fix Koopa Race Music", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("FixKoopaRaceMusic"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Fixes the Koopa race music on Bob-omb Battlefield and Tiny-Huge Island."));

    path = { "Enhancements", "Cheats", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Infinite Health", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("InfiniteHealth"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Impervious to Damage."));
    AddWidget(path, "Infinite Lives", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("InfiniteLives"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Never run of out Lives."));
    AddWidget(path, "Pause Exit Whenever", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("PauseExitWhenever"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Exit from anywhere using the Pause Menu."));
}

} // namespace GhostshipGui
