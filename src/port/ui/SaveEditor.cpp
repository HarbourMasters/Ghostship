#include "SaveEditor.h"
#include "UIWidgets.hpp"

#include <string>
#include <imgui.h>
#include <libultraship/libultraship.h>
#include "GhostshipGui.hpp"
#include "port/ui/cvar_prefixes.h"

extern "C" {
#include "game/save_file.h"
extern s16 gCurrSaveFileNum;
extern SaveBuffer gSaveBuffer;
extern MarioState* gMarioState;
}

#define DEFINE_COURSE(_0, _1, name) name,
#define DEFINE_COURSES_END()
#define DEFINE_BONUS_COURSE(_0, _1, name) name,
static char courseNames[][31] = {
    #include "levels/course_defines.h"
};
#undef DEFINE_COURSE
#undef DEFINE_COURSES_END
#undef DEFINE_BONUS_COURSE

void DrawFlagTableArray32(const FlagTable& flagTable, uint16_t row, uint32_t& flags) {
    ImGui::PushID((std::to_string(row) + flagTable.name).c_str());
    for (int32_t flagIndex = 0; flagIndex < 32; flagIndex++) {
        if ((flagIndex % 8) != 0) {
          ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        bool hasDescription = !!flagTable.flagDescriptions.contains(flagIndex);
        uint32_t bitMask = 1 << flagIndex;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, hasDescription ? ImVec4(0.16f, 0.29f, 0.48f, 0.54f) : ImVec4(0.16f, 0.29f, 0.48f, 0.24f));
        bool flag = (flags & bitMask) != 0;
        if (ImGui::Checkbox("##check", &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && hasDescription) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", UIWidgets::WrappedText(flagTable.flagDescriptions.at(flagIndex), 60));
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void SaveEditorWindow::DrawElement() {
    if (CVarGetInteger(CVAR_WINDOW("SaveEditor"), 0)) {
        ImGui::SetNextWindowSize(ImVec2(497, 532), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Save Editor")) {

            ImGui::BeginTabBar("##saveEditorTabs");
            if (ImGui::BeginTabItem("Main Save & Mario Flags")) {
                ImGui::Text("Mario Flags");
                DrawFlagTableArray32(flagTables[1], 0, gMarioState->flags);
                ImGui::Text("Save File Flags");
                DrawFlagTableArray32(flagTables[0], 0,
                                     gSaveBuffer.files[gCurrSaveFileNum - 1][0].flags);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Course Stars & Coins")) {
                for (int i = 0; i < COURSE_COUNT; i++) {
                    ImGui::Text("%s", courseNames[i]);
                    std::string invisibleLabelStr = "##courseStars" + std::string(courseNames[i]);
                    const char *invisibleLabel = invisibleLabelStr.c_str();
                    UIWidgets::DrawFlagArray8(
                        invisibleLabel,
                        gSaveBuffer.files[gCurrSaveFileNum - 1][0].courseStars[COURSE_NUM_TO_INDEX(i)]);
                    if (i < COURSE_STAGES_COUNT) {
                        ImGui::SameLine();
                        std::string invisibleLabelStr2 = "##courseCoins" + std::string(courseNames[i]);
                        const char *invisibleLabel2 = invisibleLabelStr2.c_str();
                        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4);
                        ImGui::InputScalar(invisibleLabel2, ImGuiDataType_U8,
                                           &gSaveBuffer.files[gCurrSaveFileNum - 1][0]
                                                .courseCoinScores[COURSE_NUM_TO_INDEX(i)],
                                           NULL, NULL, "%u");
                    }
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
