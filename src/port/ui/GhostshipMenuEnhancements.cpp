#include "GhostshipMenu.h"

#define CVAR_INT_SHIP_INIT(cvar, val) \
    CVarSetInteger(cvar, val);        \
    ShipInit::Init(cvar);

static std::string comboboxTooltip = "";

namespace GhostshipGui {

extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;

static std::unordered_map<int32_t, const char*> hudAspects = {
    { 0, "Expand" },
    { 1, "Custom" },
    { 2, "Original (4:3)" },
    { 3, "Widescreen (16:9)" },
    { 4, "Nintendo 3DS (5:3)" },
    { 5, "16:10 (8:5)" },
    { 6, "Ultrawide (21:9)" },
};

static std::unordered_map<int32_t, const char*> cameraModes = {
    { 0, "Lakitu" },
    { 1, "Mario" },
    { 2, "Manual" },
};

using namespace UIWidgets;

void GhostshipMenu::AddMenuEnhancements() {
    // Add Enhancements Menu
    AddMenuEntry("Enhancements", CVAR_SETTING("Menu.EnhancementsSidebarSection"));

    // Quality of Life
    WidgetPath path = { "Enhancements", "Graphics", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", "Graphics", 3);

    AddWidget(path, "Mods", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Use Alternate Assets", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Mods.AlternateAssets")
        .Options(CheckboxOptions().Tooltip(
            "Toggle between standard assets and alternate assets. Usually mods will indicate if "
            "this setting has to be used or not."));

    AddWidget(path, "Game", WIDGET_SEPARATOR_TEXT);

    AddWidget(path, "Disable Draw Distance", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("DisableDrawDistance"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Disables the draw distance, rendering all objects immediately."));

    AddWidget(path, "Show Power Meter Always", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("AlwaysShowPowerMeter"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("The power meter will always be visible on screen."));

    AddWidget(path, "HUD Aspect Ratio", WIDGET_CVAR_COMBOBOX)
        .CVar("gHUDAspectRatio.Selection")
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .ComboMap(hudAspects)
                     .Tooltip("Changes the aspect ratio of the HUD elements.")
                     .DefaultIndex(0)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Far))
        .Callback([](WidgetInfo& info) {
            CVarSetInteger("gHUDAspectRatio.Enabled", 1);
            switch (CVarGetInteger("gHUDAspectRatio.Selection", 0)) {
                case 0:
                    CVarSetInteger("gHUDAspectRatio.Enabled", 0);
                    CVarSetInteger("gHUDAspectRatio.X", 0);
                    CVarSetInteger("gHUDAspectRatio.Y", 0);
                    break;
                case 1:
                    if (CVarGetInteger("gHUDAspectRatio.X", 0) <= 0) {
                        CVarSetInteger("gHUDAspectRatio.X", 1);
                    }
                    if (CVarGetInteger("gHUDAspectRatio.Y", 0) <= 0) {
                        CVarSetInteger("gHUDAspectRatio.Y", 1);
                    }
                    break;
                case 2:
                    CVarSetInteger("gHUDAspectRatio.X", 4);
                    CVarSetInteger("gHUDAspectRatio.Y", 3);
                    break;
                case 3:
                    CVarSetInteger("gHUDAspectRatio.X", 16);
                    CVarSetInteger("gHUDAspectRatio.Y", 9);
                    break;
                case 4:
                    CVarSetInteger("gHUDAspectRatio.X", 5);
                    CVarSetInteger("gHUDAspectRatio.Y", 3);
                    break;
                case 5:
                    CVarSetInteger("gHUDAspectRatio.X", 8);
                    CVarSetInteger("gHUDAspectRatio.Y", 5);
                    break;
                case 6:
                    CVarSetInteger("gHUDAspectRatio.X", 21);
                    CVarSetInteger("gHUDAspectRatio.Y", 9);
                    break;
            }
        });

    AddWidget(path, "Custom HUD Aspect Ratio X", WIDGET_CVAR_SLIDER_INT)
        .CVar("gHUDAspectRatio.X")
        .RaceDisable(false)
        .PreFunc([](WidgetInfo& info) {
            if (CVarGetInteger("gHUDAspectRatio.Selection", 0) != 1) {
                info.isHidden = true;
            } else {
                info.isHidden = false;
            }
        })
        .Options(IntSliderOptions().Min(1).Max(100).DefaultValue(16).ShowButtons(true).Format("%d"));
    AddWidget(path, "Custom HUD Aspect Ratio Y", WIDGET_CVAR_SLIDER_INT)
        .CVar("gHUDAspectRatio.Y")
        .RaceDisable(false)
        .PreFunc([](WidgetInfo& info) {
            if (CVarGetInteger("gHUDAspectRatio.Selection", 0) != 1) {
                info.isHidden = true;
            } else {
                info.isHidden = false;
            }
        })
        .Options(IntSliderOptions().Min(1).Max(100).DefaultValue(9).ShowButtons(true).Format("%d"));

    path = { "Enhancements", "Gameplay", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Camera", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Default Camera Mode", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.DefaultMode"))
        .RaceDisable(false)
        .Options(ComboboxOptions().Tooltip("Primary camera mode.").ComboMap(cameraModes).DefaultIndex(0));
    AddWidget(path, "Alternate Camera Mode", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.AlternateMode"))
        .RaceDisable(false)
        .Options(
            ComboboxOptions().Tooltip("Alternate camera (R-trigger toggle)").ComboMap(cameraModes).DefaultIndex(0));
    AddWidget(path, "Horizontal Analog Camera", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.HorizontalAnalog"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Enable horizontal analog camera"));
    AddWidget(path, "Vertical analog camera", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.VerticalAnalog"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Enable vertical analog camera"));
    AddWidget(path, "Improved C-button Camera", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.ImprovedCButtonCamera"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Enables smooth C-button rotation (hold = continuous)"));
    AddWidget(path, "Center camera button", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.CenterCameraButton"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("L-trigger centers camera behind Mario"));
    AddWidget(path, "Inverted horizontal camera", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.InvertedHorizontalCamera"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Invert horizontal camera"));
    AddWidget(path, "Inverted vertical camera", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.InvertedVerticalCamera"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Invert vertical camera"));
    AddWidget(path, "Camera Speed", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_ENHANCEMENT("Camera.CameraSpeed"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(1.0).Max(100.0).DefaultValue(32.0).ShowButtons(true).Tooltip(
            "Analog camera sensitivity multiplier"));
    AddWidget(path, "Camera distance", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_ENHANCEMENT("Camera.CameraDistance"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(1.0).Max(1000.0).DefaultValue(100.0).ShowButtons(true).Tooltip(
            "Normal zoom distance"));
    AddWidget(path, "Camera distance zoomed out", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_ENHANCEMENT("Camera.CameraDistanceZoomedOut"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(1.0).Max(1000.0).DefaultValue(150.0).ShowButtons(true).Tooltip(
            "Zoomed-out distance"));
    AddWidget(path, "Additional camera distance", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_ENHANCEMENT("Camera.AdditionalCameraDistance"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().Min(1.0).Max(1000.0).DefaultValue(0.0).ShowButtons(true).Tooltip(
            "Extra uniform distance offset"));
    AddWidget(path, "Manual camera sounds", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Camera.ManualCameraSounds"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Play sounds when using manual camera controls (C-buttons, L-trigger)"));

    AddWidget(path, "Miscellaneous", WIDGET_SEPARATOR_TEXT);
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
    AddWidget(path, "Skip Intro Lakitu", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("DisableLakituCutscene"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("The Lakitu will not appear when Mario reaches the bridge outside Peach's \n"
                                           "Castle. Also skips Bowser's message when Mario first enters the castle."));

    path = { "Enhancements", "Fixes", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Fix Koopa Race Music", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("FixKoopaRaceMusic"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Fixes the Koopa race music on Bob-omb Battlefield and Tiny-Huge Island."));

    path = { "Enhancements", "Modes", SECTION_COLUMN_1 };
    AddSidebarEntry("Enhancements", path.sidebarName, 1);
    path.column = SECTION_COLUMN_1;

    AddWidget(path, "Mirrored World", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENHANCEMENT("Modes.MirroredWorld.Mode"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Mirrors the world horizontally. Inverts left/right controls to match."));

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
    AddWidget(path, "Play In Demo", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("PlayInDemo"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Allows normal gameplay when a demo is playing."));
    AddWidget(path, "Always Fly on Triple Jump", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("AlwaysFlyTripleJump"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Triple jumping always starts flying, as if you always have the Wing Cap."));
    AddWidget(path, "Triple Jump High Launch", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_CHEAT("FlyingTripleJumpHighLaunch"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Flying triple jump launches 3x higher than normal."));
}

} // namespace GhostshipGui
