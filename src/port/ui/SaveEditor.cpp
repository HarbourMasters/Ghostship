#include "SaveEditor.h"
#include "UIWidgets.hpp"
#include "port/ui/Notification.h"
#include "port/ShipUtils.h"

#include "port/Rando/Rando.h"
#include "port/Rando/Logic/Logic.h"
#include "port/Rando/CustomItem/CustomItem.h"
#include "port/Rando/StaticData/StaticData.h"

#include <string>
#include <imgui.h>
#include <libultraship/libultraship.h>
#include "GhostshipGui.hpp"
#include "port/ui/cvar_prefixes.h"

extern "C" {
#include "game/save_file.h"
#include "include/assets/textures/segment2.h"
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
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
                              hasDescription ? ImVec4(0.16f, 0.29f, 0.48f, 0.54f) : ImVec4(0.16f, 0.29f, 0.48f, 0.24f));
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
            ImGui::Text("%s", UIWidgets::WrappedText(flagTable.flagDescriptions.at(flagIndex), 60).c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void SaveEditorWindow::DrawElement() {
    ImGui::BeginTabBar("##saveEditorTabs");
    if (ImGui::BeginTabItem("Main Save & Mario Flags")) {
        ImGui::Text("Mario Flags");
        DrawFlagTableArray32(flagTables[1], 0, gMarioState->flags);
        ImGui::Text("Save File Flags");
        DrawFlagTableArray32(flagTables[0], 0, gSaveBuffer.files[gCurrSaveFileNum - 1][0].flags);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Course Stars & Coins")) {
        for (int i = 0; i < COURSE_COUNT; i++) {
            ImGui::Text("%s", courseNames[i]);
            std::string invisibleLabelStr = "##courseStars" + std::string(courseNames[i]);
            const char* invisibleLabel = invisibleLabelStr.c_str();
            UIWidgets::DrawFlagArray8(invisibleLabel,
                                      gSaveBuffer.files[gCurrSaveFileNum - 1][0].courseStars[COURSE_NUM_TO_INDEX(i)]);
            if (i < COURSE_STAGES_COUNT) {
                ImGui::SameLine();
                std::string invisibleLabelStr2 = "##courseCoins" + std::string(courseNames[i]);
                const char* invisibleLabel2 = invisibleLabelStr2.c_str();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4);
                ImGui::InputScalar(invisibleLabel2, ImGuiDataType_U8,
                                   &gSaveBuffer.files[gCurrSaveFileNum - 1][0].courseCoinScores[COURSE_NUM_TO_INDEX(i)],
                                   NULL, NULL, "%u");
            }
        }
        ImGui::EndTabItem();
    }

    if (!Rando::Logic::shuffledPool.empty()) {
        if (ImGui::BeginTabItem("Rando")) {
            if (ImGui::BeginTable("Rando Save Editor", 3)) {
                ImGui::TableSetupColumn("Obtained", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                ImGui::TableSetupColumn("Check Name");
                ImGui::TableSetupColumn("Item Name");

                ImGui::TableNextColumn();
                for (auto& entry : Rando::Logic::shuffledPool) {
                    if (entry.randoCheckId == RC_UNKNOWN) {
                        continue;
                    }
                    ImGui::PushID(entry.randoCheckId);
                    if (UIWidgets::Checkbox("##obtained", &entry.obtained)) {
                        bool toggleTo = entry.obtained;

                        Rando::StaticData::RandoStaticCheck randoStaticCheck =
                            Rando::StaticData::Checks[entry.randoCheckId];
                        int16_t courseNumber = Ship_GetCourseByLevel(randoStaticCheck.levelId);

                        RANDO_SAVE_CHECKS(selectedFileNum)[entry.randoCheckId].obtained = toggleTo;
                        if (entry.randoItemId == RI_STAR) {
                            if (courseNumber == COURSE_NONE) {}
                            if (toggleTo) {
                                if (courseNumber == COURSE_NONE) {
                                    gSaveBuffer.files[selectedFileNum][0].flags |= 1 << entry.randoAct;
                                } else {
                                    gSaveBuffer.files[selectedFileNum][0].courseStars[courseNumber] |=
                                        1 << entry.randoAct;
                                }
                            } else {
                                if (courseNumber == COURSE_NONE) {
                                    gSaveBuffer.files[selectedFileNum][0].flags &= ~1 << entry.randoAct;
                                } else {
                                    gSaveBuffer.files[selectedFileNum][0].courseStars[courseNumber] &=
                                        ~1 << entry.randoAct;
                                }
                            }
                        }
                        if ((entry.randoItemId == RI_COIN_BLUE || entry.randoItemId == RI_COIN_RED) &&
                            randoStaticCheck.levelId == gCurrLevelNum) {
                            int16_t coinChange = entry.randoItemId == RI_COIN_BLUE ? 5 : 2;
                            if (toggleTo) {
                                gMarioState->numCoins += coinChange;
                            } else {
                                gMarioState->numCoins -= coinChange;
                                if (gMarioState->numCoins < 0) {
                                    gMarioState->numCoins = 0;
                                }
                            }
                        }
                        gMarioState->numStars =
                            save_file_get_total_star_count(selectedFileNum, COURSE_MIN - 1, COURSE_MAX - 1);
                        gSaveFileModified = true;
                        save_file_do_save(selectedFileNum);

                        Notification::Emit({ .itemIcon = entry.randoItemId == RI_STAR       ? texture_hud_char_star
                                                         : entry.randoItemId == RI_COIN_RED ? "Red Coin Icon"
                                                                                            : "Blue Coin Icon",
                                             .message = Rando::StaticData::Items[entry.randoItemId].name,
                                             .suffix = toggleTo ? "obtained." : "removed." });
                    }
                    ImGui::TableNextColumn();
                    ImGui::TextColored(entry.obtained ? UIWidgets::ColorValues.at(UIWidgets::Colors::Green)
                                                      : UIWidgets::ColorValues.at(UIWidgets::Colors::White),
                                       Rando::StaticData::Checks[entry.randoCheckId].name);
                    ImGui::TableNextColumn();
                    ImGui::Text(Rando::StaticData::Items[entry.randoItemId].name);
                    ImGui::PopID();
                    ImGui::TableNextColumn();
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
}
