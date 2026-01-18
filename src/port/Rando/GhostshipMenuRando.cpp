#include "port/ui/GhostshipMenu.h"
#include "port/Rando/Rando.h"

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
using namespace UIWidgets;

void GhostshipMenu::AddMenuRando() {
    // Add Rando Menu
    AddMenuEntry("Rando", CVAR_SETTING("Menu.RandoSidebarSection"));

    WidgetPath path = { "Rando", "General", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Randomizer Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Enable Rando", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("Enabled"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Enables the randomizer feature."));
    AddWidget(path, "Logic Type", WIDGET_CVAR_COMBOBOX)
        .CVar(Rando::StaticData::Options[RO_LOGIC].cvar)
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .Tooltip("Sets the Logic type for the seed.")
                     .ComboMap(Rando::StaticData::logicOptions)
                     .DefaultIndex(RO_LOGIC_GLITCHLESS));

    path = { "Rando", "Shuffle Options", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Item Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Shuffle Stars", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_STARS].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Stars into the Item Pool."));
    AddWidget(path, "Shuffle Red Coins", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_COINS_RED].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Red Coins into the Item Pool."));
    AddWidget(path, "Shuffle Blue Coins", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_COINS_BLUE].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Blue Coins into the Item Pool."));
}

} // namespace GhostshipGui
