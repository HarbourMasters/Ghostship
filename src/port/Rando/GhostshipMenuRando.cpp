#include "port/ui/GhostshipMenu.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/ui/Notification.h"

#define WIDGET_COLOR UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5))

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
extern std::shared_ptr<Rando::CheckTracker::CheckTrackerWindow> mRandoCheckTrackerWindow;
extern std::shared_ptr<Rando::CheckTracker::SettingsWindow> mRandoCheckTrackerSettingsWindow;
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
                     .DefaultIndex(RO_LOGIC_GLITCHLESS)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Near));

    AddWidget(path, "SeperatorBar", WIDGET_SEPARATOR);

    AddWidget(path, "Generate Spoiler Log", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("GenerateLog"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Generates a Spoiler Log in the randomizer folder.").DefaultValue(true));
    // TODO: populate combobox with existing spoiler logs
    AddWidget(path, "Load Existing Spoiler Log", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("UseExistingLog"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Uses a Spoiler Log in the randomizer folder."))
        .PreFunc([](WidgetInfo& info) {
            info.options->Disabled(CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 0));
        });
    AddWidget(path, "Manual Seed Entry", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Generates a seed using the provided input."));
    AddWidget(path, "Seed", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        ImGui::BeginDisabled(!CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 0));
        UIWidgets::PushStyleInput(WIDGET_COLOR);
        ImGui::InputText("##ManualSeed", seedString, MAX_SEED_STRING_SIZE, ImGuiInputTextFlags_CallbackCharFilter,
                         UIWidgets::TextFilters::FilterAlphaNum);
        UIWidgets::Tooltip("Characters from a-z, A-Z, and 0-9 are supported.\n"
                           "Character limit is 1023, after which the seed will be truncated.\n");
        ImGui::SameLine();
        if (UIWidgets::Button(ICON_FA_ERASER, UIWidgets::ButtonOptions()
                                                  .Size(UIWidgets::Sizes::Inline)
                                                  .Color(WIDGET_COLOR)
                                                  .Padding(ImVec2(10.f, 6.f)))) {
            memset(seedString, 0, MAX_SEED_STRING_SIZE);
        }
        if (strnlen(seedString, MAX_SEED_STRING_SIZE) == 0) {
            ImGui::SameLine(17.0f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Leave blank for random seed");
        }
        UIWidgets::PopStyleInput();
        ImGui::EndDisabled();
    });
    AddWidget(path, "Create Spoiler Log", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) {
            nlohmann::json spoiler = Rando::Spoiler::GenerateFromPoolGeneration(Rando::Logic::shuffledPool);
            std::string fileName = spoiler["fileNum"].get<std::string>() + ".json";
            Rando::Spoiler::SaveToFile(fileName, spoiler);
            Notification::Emit({ .prefix = fileName + " ",
                                 .message = "Spoiler Log created.",
                                 .messageColor = ImVec4(0, 0.3f, 0.85f, 1) });
        })
        .Options(ButtonOptions().Tooltip("Creates a Spoiler Log from the current SaveFile."))
        .PreFunc([](WidgetInfo& info) { info.options->Disabled(Rando::Logic::shuffledPool.empty()); });

    AddWidget(path, "SeperatorBar", WIDGET_SEPARATOR);

    path = { "Rando", "Enhancements", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Skip Get Item Cutscene", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("SkipRandoGI"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Skips the cutscene when collecting a Star."));

    path = { "Rando", "Shuffle Options", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Item Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Shuffle Stars", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_STARS].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Stars into the Item Pool."));
    AddWidget(path, "Shuffle Red Coin Stars", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_RED_COIN_STARS].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Stars spawned from collecting 8 Red Coins into the Item Pool."))
        .PreFunc(
            [](WidgetInfo& info) { info.options->Disabled(Rando::StaticData::Options[RO_SHUFFLE_STARS].cvar == 0); });
    AddWidget(path, "Shuffle Red Coins", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_COINS_RED].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Red Coins into the Item Pool."));
    AddWidget(path, "Shuffle Blue Coins", WIDGET_CVAR_CHECKBOX)
        .CVar(Rando::StaticData::Options[RO_SHUFFLE_COINS_BLUE].cvar)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Shuffles Blue Coins into the Item Pool."));

    path = { "Rando", "Check Tracker", SECTION_COLUMN_1 };
    AddSidebarEntry("Rando", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Popout Settings", WIDGET_WINDOW_BUTTON)
        .CVar("gWindows.CheckTrackerSettings")
        .WindowName("Check Tracker Settings");
}

} // namespace GhostshipGui
