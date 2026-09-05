#include "EntranceTracker.h"
#include <ship/window/gui/IconsFontAwesome4.h>
#include "port/ShipCompat.h"
#include "port/Rando/Logic/Logic.h"
#include "port/ShipUtils.h"
#include "port/ui/UIWidgets.hpp"
#include <cstring>

namespace GhostshipGui {
extern std::shared_ptr<Rando::EntranceTracker::EntranceTrackerWindow> mRandoEntranceTrackerWindow;
}

#define DEFAULT_FOUND_COLOR \
    Color_RGBA8 {           \
        100, 255, 100, 255  \
    }

#define CVAR_NAME_SHOW_ENTRANCE_TRACKER "gWindows.EntranceTracker"
#define CVAR_NAME_ENTRANCE_TRACKER_OPACITY "gRando.EntranceTracker.Opacity"
#define CVAR_NAME_ENTRANCE_TRACKER_SCALE "gRando.EntranceTracker.Scale"
#define CVAR_NAME_FOUND_COLOR "gRando.EntranceTracker.FoundColor"

#define CVAR_SHOW_ENTRANCE_TRACKER CVarGetInteger(CVAR_NAME_SHOW_ENTRANCE_TRACKER, 0)
#define CVAR_ENTRANCE_TRACKER_OPACITY CVarGetFloat(CVAR_NAME_ENTRANCE_TRACKER_OPACITY, 0.5f)
#define CVAR_ENTRANCE_TRACKER_SCALE CVarGetFloat(CVAR_NAME_ENTRANCE_TRACKER_SCALE, 1.0f)
#define CVAR_FOUND_COLOR CVarGetColor(CVAR_NAME_FOUND_COLOR ".Value", DEFAULT_FOUND_COLOR)

std::vector<std::tuple<const char*, Color_RGBA8, const char*>> defaultEntranceColorList = {
    { CVAR_NAME_FOUND_COLOR, DEFAULT_FOUND_COLOR, "Entrance Found" },
};

bool entranceTrackerPopoutState = false;
ImVec4 entranceTrackerBG = ImVec4{ 0, 0, 0, 0.5f };
float entranceTrackerScale = 1.0f;

void DrawEntranceTrackerList() {
    if (Rando::Logic::shuffledEntrances.empty()) {
        return;
    }

    if (ImGui::BeginTable("EntranceList", 2)) {
        ImGui::TableNextColumn();
        for (auto& [entranceId, destinationId, isFound] : Rando::Logic::shuffledEntrances) {
            if (entranceId == RE_UNKNOWN) {
                continue;
            }

            const char* entranceName = levelIdList.at(Rando::StaticData::Entrances[entranceId].destinationId).c_str();
            const char* destinationName = levelIdList.at(destinationId).c_str();
            ImVec4 foundTextColor = VecFromRGBA8(CVAR_FOUND_COLOR);

            ImGui::Text("%s", entranceName);
            ImGui::TableNextColumn();
            if (isFound) {
                ImGui::TextColored(foundTextColor, "(%s)", destinationName);
            }
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    }
}

namespace Rando {

namespace EntranceTracker {

void EntranceTrackerWindow::Draw() {
    if (!CVAR_SHOW_ENTRANCE_TRACKER) {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, entranceTrackerBG);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, entranceTrackerBG);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, entranceTrackerBG);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

    ImGui::SetNextWindowSize(ImVec2(485.0f, 500.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Entrance Tracker", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::SetWindowFontScale(entranceTrackerScale);

        if (Rando::Logic::shuffledEntrances.empty()) {
            ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), "No Rando Save Loaded");
            ImGui::End();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(1);
            return;
        }

        if (ImGui::BeginChild("Entrances")) {
            DrawEntranceTrackerList();
            ImGui::EndChild();
        }

        ImGui::End();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(1);
    }
}

void SettingsWindow::DrawElement() {
    if (CVarGetInteger("gWindows.EntranceTracker", 0)) {
        entranceTrackerPopoutState = true;
        UIWidgets::WindowButton("Return Entrance Tracker", "gWindows.EntranceTracker",
                                GhostshipGui::mRandoEntranceTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red });
    } else {
        entranceTrackerPopoutState = false;
        UIWidgets::WindowButton("Popout Entrance Tracker", "gWindows.EntranceTracker",
                                GhostshipGui::mRandoEntranceTrackerWindow,
                                { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green });
    }
    if (ImGui::BeginTable("Settings Table", 2)) {
        ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("col2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextColumn();

        ImGui::SeparatorText("Entrance Tracker");
        if (!entranceTrackerPopoutState) {
            if (ImGui::BeginChild("Entrances", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                DrawEntranceTrackerList();
                ImGui::EndChild();
            }
        } else {
            ImGui::TextColored(UIWidgets::ColorValues.at(WIDGET_COLOR), "Tracker popped out");
        }

        ImGui::TableNextColumn();
        ImGui::SeparatorText("Window Settings");
        // UIWidgets::CVarCheckbox("Show Search", CVAR_NAME_SHOW_SEARCH,

        if (UIWidgets::CVarSliderFloat("", CVAR_NAME_ENTRANCE_TRACKER_OPACITY,
                                       {
                                           .format = "Opacity: %.1f",
                                           .step = 0.10f,
                                           .min = 0.0f,
                                           .max = 1.0f,
                                           .defaultValue = 0.5f,
                                           .labelPosition = UIWidgets::LabelPositions::None,
                                           .color = WIDGET_COLOR,
                                       })) {
            entranceTrackerBG.w = CVAR_ENTRANCE_TRACKER_OPACITY;
        }

        if (UIWidgets::CVarSliderFloat(" ", CVAR_NAME_ENTRANCE_TRACKER_SCALE,
                                       {
                                           .format = "Scale: %.1f",
                                           .step = 0.10f,
                                           .min = 0.7f,
                                           .max = 2.5f,
                                           .defaultValue = 1.0f,
                                           .labelPosition = UIWidgets::LabelPositions::None,
                                           .color = WIDGET_COLOR,
                                       })) {
            entranceTrackerScale = CVAR_ENTRANCE_TRACKER_SCALE;
        }

        int16_t entranceColorIndex = 0;
        for (auto& [cvar, color, label] : defaultEntranceColorList) {
            std::string cvarText = cvar;
            cvarText += ".Value";
            std::string colorText = label;
            colorText += " Color";
            std::string widgetLabel = "##";
            widgetLabel += std::to_string(entranceColorIndex);

            ImGui::PushID(entranceColorIndex);
            UIWidgets::CVarColorPicker(widgetLabel.c_str(), cvar, color, true);
            ImGui::SameLine();
            if (UIWidgets::Button(ICON_FA_REFRESH, { .size = ImVec2(32.0f, 32.0f), .color = WIDGET_COLOR })) {
                CVarSetColor(cvarText.c_str(), color);
                ShipCompat::GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
            }
            ImGui::SameLine();
            ImGui::Text("%s", colorText.c_str());
            ImGui::PopID();
            entranceColorIndex++;
        }

        ImGui::EndTable();
    }
}

void Init() {
    entranceTrackerPopoutState = CVarGetInteger("gWindows.EntranceTracker", 0);
    entranceTrackerBG = { 0, 0, 0, CVAR_ENTRANCE_TRACKER_OPACITY };
    entranceTrackerScale = CVAR_ENTRANCE_TRACKER_SCALE;
}

} // namespace EntranceTracker
} // namespace Rando