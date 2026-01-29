#include "CheckTracker.h"
#include "port/Rando/Logic/Logic.h"
#include "port/ShipUtils.h"
#include "port/ui/UIWidgets.hpp"
#include <cstring>

extern "C" {
#include "include/assets/textures/segment2.h"
}

namespace GhostshipGui {
extern std::shared_ptr<Rando::CheckTracker::CheckTrackerWindow> mRandoCheckTrackerWindow;
}

#define WIDGET_COLOR UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5))
#define DEFAULT_COLLECTED_COLOR Color_RGBA8(100, 255, 100, 255)
#define DEFAULT_SKIPPED_COLOR Color_RGBA8(255, 100, 255, 255)
#define DEFAULT_ITEM_COLOR Color_RGBA8(79, 0, 221, 255)

#define CVAR_NAME_SHOW_CHECK_TRACKER "gWindows.CheckTracker"
#define CVAR_NAME_TRACKER_OPACITY "gRando.CheckTracker.Opacity"
#define CVAR_NAME_TRACKER_SCALE "gRando.CheckTracker.Scale"
#define CVAR_NAME_SHOW_CURRENT_LEVEL "gRando.CheckTracker.ShowCurrentLevel"
#define CVAR_NAME_COLLECTED_COLOR "gRando.CheckTracker.CollectedColor"
#define CVAR_NAME_SKIPPED_COLOR "gRando.CheckTracker.SkippedColor"
#define CVAR_NAME_ITEM_COLOR "gRando.CheckTracker.ItemColor"

#define CVAR_SHOW_CHECK_TRACKER CVarGetInteger(CVAR_NAME_SHOW_CHECK_TRACKER, 0)
#define CVAR_TRACKER_OPACITY CVarGetFloat(CVAR_NAME_TRACKER_OPACITY, 0.5f)
#define CVAR_TRACKER_SCALE CVarGetFloat(CVAR_NAME_TRACKER_SCALE, 1.0f)
#define CVAR_SHOW_CURRENT_LEVEL CVarGetInteger(CVAR_NAME_SHOW_CURRENT_LEVEL, 0)
#define CVAR_COLLECTED_COLOR CVarGetColor(CVAR_NAME_COLLECTED_COLOR ".Value", DEFAULT_COLLECTED_COLOR)
#define CVAR_SKIPPED_COLOR CVarGetColor(CVAR_NAME_SKIPPED_COLOR ".Value", DEFAULT_SKIPPED_COLOR)
#define CVAR_ITEM_COLOR CVarGetColor(CVAR_NAME_ITEM_COLOR ".Value", DEFAULT_ITEM_COLOR)

std::vector<std::tuple<const char*, Color_RGBA8, const char*>> defaultColorList = {
    { CVAR_NAME_COLLECTED_COLOR, DEFAULT_COLLECTED_COLOR, "Check Obtained" },
    { CVAR_NAME_SKIPPED_COLOR, DEFAULT_SKIPPED_COLOR, "Check Skipped" },
    { CVAR_NAME_ITEM_COLOR, DEFAULT_ITEM_COLOR, "Obtained Item" },
};

std::map<int16_t, std::string> levelIdList = {
    { LEVEL_BOB, "Bob Omb Battlefield" },
    { LEVEL_WF, "Whomp's Fortress" },
    { LEVEL_JRB, "Jolly Rodger's Bay" },
    { LEVEL_CASTLE, "Castle Interior" },
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

bool trackerPopoutState = false;
ImVec4 trackerBG = ImVec4{ 0, 0, 0, 0.5f };
float trackerScale = 1.0f;

bool expandState = true;

void DrawCheckTrackerList() {
    if (Rando::Logic::shuffledPool.empty()) {
        return;
    }
    for (auto& [id, name] : levelIdList) {
        if (CVAR_SHOW_CURRENT_LEVEL && id != gCurrLevelNum) {
            continue;
        }

        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0.5f));
        ImGui::SetNextItemOpen(expandState, ImGuiCond_Always);
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(20.0f);
            if (ImGui::BeginTable("CheckList", 2)) {
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, (16.0f * trackerScale));
                ImGui::TableSetupColumn("Check");

                ImGui::TableNextColumn();
                for (auto& entry : Rando::Logic::shuffledPool) {
                    RandoSaveCheck& randoSaveCheck = RANDO_SAVE_CHECKS(selectedFileNum)[entry.randoCheckId];
                    if (Rando::StaticData::Checks[entry.randoCheckId].levelId != id) {
                        continue;
                    }
                    ImVec4 checkTextColor = randoSaveCheck.obtained
                                                ? VecFromRGBA8(CVAR_COLLECTED_COLOR)
                                                : UIWidgets::ColorValues.at(UIWidgets::Colors::White);
                    ImVec4 itemTextColor = randoSaveCheck.obtained
                                               ? VecFromRGBA8(CVAR_ITEM_COLOR)
                                               : UIWidgets::ColorValues.at(UIWidgets::Colors::Indigo);
                    if (randoSaveCheck.skipped) {
                        checkTextColor = itemTextColor = VecFromRGBA8(CVAR_SKIPPED_COLOR);
                    }
                    const char* texture = randoSaveCheck.randoItemId == RI_STAR       ? texture_hud_char_star
                                          : randoSaveCheck.randoItemId == RI_COIN_RED ? "Red Coin Icon"
                                                                                      : "Blue Coin Icon";
                    ImTextureID textureId =
                        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(texture);

                    ImGui::BeginGroup();
                    ImGui::Image(textureId, ImVec2(16.0f * trackerScale, 16.0f * trackerScale));
                    ImGui::TableNextColumn();
                    ImGui::TextColored(checkTextColor, Rando::StaticData::Checks[entry.randoCheckId].name);
                    if (randoSaveCheck.obtained) {
                        ImGui::SameLine();
                        RandoItemId randoItemId = Rando::StaticData::GetShuffledRandoItem(entry.randoCheckId);
                        ImGui::TextColored(itemTextColor, "(%s)", Rando::StaticData::Items[randoItemId].name);
                    } else if (randoSaveCheck.skipped) {
                        ImGui::SameLine();
                        ImGui::TextColored(itemTextColor, "(%s)", "Skipped");
                    }
                    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0));
                    ImGui::EndGroup();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::IsItemHovered()
                                                                          ? IM_COL32(255, 255, 0, 128)
                                                                          : IM_COL32(255, 255, 255, 0));
                    if (ImGui::IsItemClicked()) {
                        randoSaveCheck.skipped = !randoSaveCheck.skipped;
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
        trackerPopoutState = true;
        UIWidgets::WindowButton("Return Check Tracker", "gWindows.CheckTracker", GhostshipGui::mRandoCheckTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red });
    } else {
        trackerPopoutState = false;
        UIWidgets::WindowButton("Popout Check Tracker", "gWindows.CheckTracker", GhostshipGui::mRandoCheckTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green });
    }
    if (ImGui::BeginTable("Settings Table", 2)) {
        ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("col2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextColumn();

        ImGui::SeparatorText("Check Tracker");
        if (!trackerPopoutState) {
            if (ImGui::BeginChild("Checks", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                DrawCheckTrackerList();
                ImGui::EndChild();
            }
        } else {
            ImGui::TextColored(UIWidgets::ColorValues.at(WIDGET_COLOR), "Tracker popped out");
        }

        ImGui::TableNextColumn();
        ImGui::SeparatorText("Check Settings");
        UIWidgets::CVarCheckbox("Only Show Current Level", CVAR_NAME_SHOW_CURRENT_LEVEL);
        if (UIWidgets::Button(
                "Expand All",
                UIWidgets::ButtonOptions{}.Color(WIDGET_COLOR).Size(ImVec2(ImGui::GetContentRegionAvail().x / 2, 0)))) {
            expandState = true;
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Collapse All", UIWidgets::ButtonOptions{}.Color(WIDGET_COLOR))) {
            expandState = false;
        }

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

        int16_t colorIndex = 0;
        for (auto& [cvar, color, label] : defaultColorList) {
            std::string cvarText = cvar;
            cvarText += ".Value";
            std::string colorText = label;
            colorText += " Color";
            std::string widgetLabel = "##";
            widgetLabel += std::to_string(colorIndex);

            ImGui::PushID(colorIndex);
            UIWidgets::CVarColorPicker(widgetLabel.c_str(), cvar, color, true);
            ImGui::SameLine();
            if (UIWidgets::Button(ICON_FA_REFRESH, { .size = ImVec2(32.0f, 32.0f), .color = WIDGET_COLOR })) {
                CVarSetColor(cvarText.c_str(), color);
                Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
            }
            ImGui::SameLine();
            ImGui::Text(colorText.c_str());
            ImGui::PopID();
            colorIndex++;
        }
        ImGui::EndTable();
    }
}

bool isInitialized = false;
void Init() {
    trackerPopoutState = CVarGetInteger("gWindows.CheckTracker", 0);
    trackerBG = { 0, 0, 0, CVAR_TRACKER_OPACITY };
    trackerScale = CVAR_TRACKER_SCALE;
}

} // namespace CheckTracker
} // namespace Rando