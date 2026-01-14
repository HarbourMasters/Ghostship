#include "port/ui/GhostshipMenu.h"

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
using namespace UIWidgets;

void GhostshipMenu::AddMenuRando() {
    // Add Rando Menu
    AddMenuEntry("Rando", CVAR_SETTING("Menu.RandoSidebarSection"));

    WidgetPath path = { "Rando", "General", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Enable Rando", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("Enabled"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Enables the randomizer feature."));
}

} // namespace GhostshipGui
