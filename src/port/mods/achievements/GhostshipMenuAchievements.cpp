#include "port/ui/GhostshipMenu.h"

namespace GhostshipGui {
using namespace UIWidgets;

void GhostshipMenu::AddMenuAchievements() {
    // Add Rando Menu
    AddMenuEntry("Achievements", CVAR_SETTING("Menu.AchievementsSidebarSection"));

    WidgetPath path = { "Achievements", "General", SECTION_COLUMN_1 };
    AddSidebarEntry("Achievements", path.sidebarName, 1);

    AddWidget(path, "Enable Achievements", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Achievements"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Enables the achievement system, which tracks various accomplishments and milestones in the game."));

    AddWidget(path, "Popout Achievements", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("Achievements"))
        .WindowName("Achievements")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Achievements Window."));
}

} // namespace GhostshipGui
