#include "CheckTracker.h"
#include "port/Rando/Logic/Logic.h"
#include "port/ShipUtils.h"
#include "port/ui/UIWidgets.hpp"
#include <cstring>

namespace GhostshipGui {
extern std::shared_ptr<Rando::CheckTracker::CheckTrackerWindow> mRandoCheckTrackerWindow;
}

#define WIDGET_COLOR UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5))

#define CVAR_NAME_SHOW_CHECK_TRACKER "gWindows.CheckTracker"
#define CVAR_NAME_TRACKER_OPACITY "gRando.CheckTracker.Opacity"
#define CVAR_NAME_TRACKER_SCALE "gRando.CheckTracker.Scale"
#define CVAR_NAME_SHOW_CURRENT_LEVEL "gRando.CheckTracker.ShowCurrentLevel"
#define CVAR_NAME_COLLECTED_COLOR "gRando.CheckTracker.CollectedColor"
#define CVAR_NAME_ITEM_COLOR "gRando.CheckTracker.ItemColor"

#define CVAR_SHOW_CHECK_TRACKER CVarGetInteger(CVAR_NAME_SHOW_CHECK_TRACKER, 0)
#define CVAR_TRACKER_OPACITY CVarGetFloat(CVAR_NAME_TRACKER_OPACITY, 0.5f)
#define CVAR_TRACKER_SCALE CVarGetFloat(CVAR_NAME_TRACKER_SCALE, 1.0f)
#define CVAR_SHOW_CURRENT_LEVEL CVarGetInteger(CVAR_NAME_SHOW_CURRENT_LEVEL, 0)
#define CVAR_COLLECTED_COLOR CVarGetColor(CVAR_NAME_COLLECTED_COLOR ".Value", { 100, 255, 100, 255 })
#define CVAR_ITEM_COLOR CVarGetColor(CVAR_NAME_ITEM_COLOR ".Value", { 79, 0, 221, 255 })

std::map<int16_t, std::string> levelIdList = {
    { LEVEL_BOB, "Bob Omb Battlefield" },
    { LEVEL_WF, "Whomp's Fortress" },
    { LEVEL_JRB, "Jolly Rodger's Bay" },
    { LEVEL_CCM, "Cool Cool Mountain" },
    { LEVEL_BBH, "Big Boo's Haunt" },
    { LEVEL_HMC, "Hazy Maze Cave" },
    { LEVEL_LLL, "Lethal Lava Land" },
    { LEVEL_SSL, "Shifting Sand Land" },
    { LEVEL_DDD, "Dire Dire Docks" },
    { LEVEL_SL, "Snowman's Land" },
    { LEVEL_WDW, "Wet Dry World" },
    { LEVEL_TTM, "Tall Tall Mountain" },
    { LEVEL_THI, "Tiny Huge Island" },
    { LEVEL_TTC, "Tick Tock Clock" },
    { LEVEL_RR, "Rainbow Ride" },
    { LEVEL_BITDW, "Bowser in the Dark World" },
    { LEVEL_BITFS, "Bowser in the Fire Sea" },
    { LEVEL_BITS, "Bowser in the Sky" },
    { LEVEL_PSS, "Princess's Secret Slide" },
    { LEVEL_COTMC, "Cavern of the Metal Cap" },
    { LEVEL_TOTWC, "Tower of the Wing Cap" },
    { LEVEL_VCUTM, "Vanish Cap Under the Moat" },
    { LEVEL_WMOTR, "Winged Mario over the Rainbow" },
    { LEVEL_SA, "Secret Aquarium" },
};

ImVec4 trackerBG = ImVec4{ 0, 0, 0, 0.5f };
float trackerScale = 1.0f;

void DrawCheckTrackerList() {
    for (auto& [id, name] : levelIdList) {
        if (CVAR_SHOW_CURRENT_LEVEL && id != gCurrLevelNum) {
            continue;
        }

        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0.5f));
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf)) {
            ImGui::Indent(20.0f);
            if (ImGui::BeginTable("CheckList", 1)) {
                ImGui::TableNextColumn();
                for (auto& entry : Rando::Logic::shuffledPool) {
                    if (Rando::StaticData::Checks[entry.randoCheckId].levelId != id) {
                        continue;
                    }
                    ImVec4 checkTextColor = entry.obtained ? VecFromRGBA8(CVAR_COLLECTED_COLOR)
                                                      : UIWidgets::ColorValues.at(UIWidgets::Colors::White);
                    ImVec4 itemTextColor = entry.obtained ? VecFromRGBA8(CVAR_ITEM_COLOR)
                                                           : UIWidgets::ColorValues.at(UIWidgets::Colors::Indigo);
                    ImGui::TextColored(checkTextColor, Rando::StaticData::Checks[entry.randoCheckId].name);
                    if (entry.obtained) {
                        ImGui::SameLine();
                        RandoItemId randoItemId = Rando::StaticData::GetShuffledRandoItem(entry.randoCheckId);
                        ImGui::TextColored(itemTextColor, "(%s)",
                                           Rando::StaticData::Items[randoItemId].name);
                    }
                    ImGui::TableNextColumn();
                }
                ImGui::EndTable();
            }
            ImGui::Unindent(20.0f);
        }
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }
}

namespace Rando {

namespace CheckTracker {

void CheckTrackerWindow::Draw() {
    if (!CVAR_SHOW_CHECK_TRACKER) {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, trackerBG);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, trackerBG);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, trackerBG);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

    ImGui::SetNextWindowSize(ImVec2(485.0f, 500.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Check Tracker", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing)) {
        trackerBG.w = ImGui::IsWindowDocked() ? 1.0f : CVAR_TRACKER_OPACITY;
        ImGui::SetWindowFontScale(trackerScale);
        if (Rando::Logic::shuffledPool.empty()) {
            ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), "No Rando Save Loaded");
            ImGui::End();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(1);
            return;
        }

        if (ImGui::BeginChild("Checks")) {
            DrawCheckTrackerList();
            ImGui::EndChild();
        }

        ImGui::End();
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(1);
}

void SettingsWindow::DrawElement() {
    if (CVarGetInteger("gWindows.CheckTracker", 0)) {
        UIWidgets::WindowButton("Disable Check Tracker", "gWindows.CheckTracker",
                                GhostshipGui::mRandoCheckTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red });
    } else {
        UIWidgets::WindowButton("Enable Check Tracker", "gWindows.CheckTracker", GhostshipGui::mRandoCheckTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green });
    }
    if (ImGui::BeginTable("Settings Table", 2)) {
        ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("col2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextColumn();
        ImGui::SeparatorText("Check Settings");
        UIWidgets::CVarCheckbox("Only Show Current Level", CVAR_NAME_SHOW_CURRENT_LEVEL);

        ImGui::TableNextColumn();
        ImGui::SeparatorText("Window Settings");
        // UIWidgets::CVarCheckbox("Show Search", CVAR_NAME_SHOW_SEARCH,
        // UIWidgets::CheckboxOptions().DefaultValue(true)); UIWidgets::CVarCheckbox("Show Check Type Filters",
        // CVAR_NAME_SHOW_CHECK_TYPE_FILTER);

        if (UIWidgets::CVarSliderFloat("", CVAR_NAME_TRACKER_OPACITY,
                                       {
                                           .format = "Opacity: %.1f",
                                           .step = 0.10f,
                                           .min = 0.0f,
                                           .max = 1.0f,
                                           .defaultValue = 0.5f,
                                           .labelPosition = UIWidgets::LabelPositions::None,
                                           .color = WIDGET_COLOR,
                                       })) {
            trackerBG.w = CVAR_TRACKER_OPACITY;
        }

        if (UIWidgets::CVarSliderFloat(" ", CVAR_NAME_TRACKER_SCALE,
                                       {
                                           .format = "Scale: %.1f",
                                           .step = 0.10f,
                                           .min = 0.7f,
                                           .max = 2.5f,
                                           .defaultValue = 1.0f,
                                           .labelPosition = UIWidgets::LabelPositions::None,
                                           .color = WIDGET_COLOR,
                                       })) {
            trackerScale = CVAR_TRACKER_SCALE;
        }

        UIWidgets::CVarColorPicker("##CollectedColor", CVAR_NAME_COLLECTED_COLOR, { 100, 255, 100, 255 }, true);
        ImGui::SameLine();
        ImGui::Text("Check Obtained Color");
        UIWidgets::CVarColorPicker("##ItemColor", CVAR_NAME_ITEM_COLOR, { 79, 0, 221, 255 }, true);
        ImGui::SameLine();
        ImGui::Text("Obtained Item Color");

        ImGui::EndTable();
    }
}

bool isInitialized = false;
void Init() {
    trackerBG = { 0, 0, 0, CVAR_TRACKER_OPACITY };
    trackerScale = CVAR_TRACKER_SCALE;
}

} // namespace CheckTracker
} // namespace Rando