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

bool shouldPopUpOpen = false;
RandoCheckId popUpId = RC_UNKNOWN;
std::map<RandoItemId, const char*> objectMap = {
    { RI_COIN_BLUE, "Blue Coin Icon" },
    { RI_COIN_RED, "Red Coin Icon" },
    { RI_STAR, texture_hud_char_star },
};

std::map<std::string, int32_t> randoFlagList = {
    { "Unlock Wing Cap", SAVE_FLAG_HAVE_WING_CAP },
    { "Unlock Metal Cap", SAVE_FLAG_HAVE_METAL_CAP },
    { "Unlock Vanish Cap", SAVE_FLAG_HAVE_VANISH_CAP },
    { "Unlock Bowser Key 1", SAVE_FLAG_HAVE_KEY_1 },
    { "Unlock Bowser Key 2", SAVE_FLAG_HAVE_KEY_2 },
};

void ModifyStarFlags(bool isObtained, int16_t courseNum, int16_t starAct, int16_t fileNum) {
    if (isObtained) {
        if (courseNum == COURSE_NONE) {
            gSaveBuffer.files[fileNum][0].flags |= STAR_FLAG_TO_SAVE_FLAG(1 << starAct);
        } else {
            gSaveBuffer.files[fileNum][0].courseStars[courseNum] |= (1 << starAct);
        }
    } else {
        if (courseNum == COURSE_NONE) {
            gSaveBuffer.files[fileNum][0].flags &= ~STAR_FLAG_TO_SAVE_FLAG(1 << starAct);
        } else {
            gSaveBuffer.files[fileNum][0].courseStars[courseNum] &= ~(1 << starAct);
        }
    }

    gSaveFileModified = true;
    gMarioState->numStars = save_file_get_total_star_count(fileNum, COURSE_MIN - 1, COURSE_MAX - 1);
    save_file_do_save(fileNum);
}

void DrawFlagTableArray32(const FlagTable& flagTable, uint16_t row, uint32_t& flags) {
    ImGui::PushID((std::to_string(row) + flagTable.name).c_str());
    for (int32_t flagIndex = 0; flagIndex < 32; flagIndex++) {
        if ((flagIndex % 8) != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        bool hasDescription = !!flagTable.flagDescriptions.contains(flagIndex);
        uint32_t bitMask = 1 << flagIndex;

        ImGui::BeginDisabled(!hasDescription);
        UIWidgets::PushStyleCheckbox(WIDGET_COLOR);
        bool flag = (flags & bitMask) != 0;
        if (UIWidgets::Checkbox("##check", &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        if (ImGui::IsItemHovered() && hasDescription) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", UIWidgets::WrappedText(flagTable.flagDescriptions.at(flagIndex), 60).c_str());
            ImGui::EndTooltip();
        }
        UIWidgets::PopStyleCheckbox();
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::PopID();
}

void RandoSaveFile() {
    gSaveFileModified = true;
    gSaveBuffer.files[selectedFileNum][0].flags |= SAVE_FLAG_FILE_EXISTS;
    save_file_do_save(selectedFileNum);
    gMarioState->numStars = save_file_get_total_star_count(selectedFileNum, COURSE_MIN - 1, COURSE_MAX - 1);
    RefreshChecksInLogic();
}

void HandlePopUpContext(RandoCheckId randoCheckId) {
    if (shouldPopUpOpen && ImGui::BeginPopup("ObjectSubMenu")) {
        for (auto& [randoItemId, textureId] : objectMap) {
            if (ImGui::ImageButton(std::to_string(randoItemId).c_str(),
                                   Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(textureId),
                                   ImVec2(32.0f, 32.0f))) {
                // randoStaticCheck.randoItemId = randoItemId;
                auto findIt =
                    std::find_if(Rando::Logic::shuffledPool.begin(), Rando::Logic::shuffledPool.end(),
                                 [&](const LevelShuffleEntry& entry) { return entry.randoCheckId == randoCheckId; });
                if (findIt != Rando::Logic::shuffledPool.end()) {
                    findIt->randoItemId = randoItemId;
                    RANDO_SAVE_CHECKS(selectedFileNum)[randoCheckId].randoItemId = randoItemId;
                    RandoSaveFile();
                }
                ImGui::CloseCurrentPopup();
                shouldPopUpOpen = false;
            }
            ImGui::SameLine();
        }
        ImGui::EndPopup();
    }
    if (shouldPopUpOpen && ImGui::BeginPopup("ActSubMenu")) {
        for (int i = 0; i < RA_ACT_MAX; i++) {
            if (ImGui::ImageButton(
                    std::to_string(i).c_str(),
                    Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(digitList[i + 1]),
                    ImVec2(32.0f, 32.0f))) {
                // randoStaticCheck.randoItemId = randoItemId;
                auto findIt =
                    std::find_if(Rando::Logic::shuffledPool.begin(), Rando::Logic::shuffledPool.end(),
                                 [&](const LevelShuffleEntry& entry) { return entry.randoCheckId == randoCheckId; });
                if (findIt != Rando::Logic::shuffledPool.end()) {
                    findIt->randoAct = (RandoAct)i;
                    RANDO_SAVE_CHECKS(selectedFileNum)[randoCheckId].randoAct = (RandoAct)i;
                    RandoSaveFile();
                }
                ImGui::CloseCurrentPopup();
                shouldPopUpOpen = false;
            }
            ImGui::SameLine();
        }
        ImGui::EndPopup();
    }
}

void SaveEditorWindow::DrawElement() {
    if (gMarioSpawnInfo->model == NULL) {
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), "No Save File Loaded");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    UIWidgets::PushStyleTabs(WIDGET_COLOR);
    ImGui::BeginTabBar("##saveEditorTabs");
    if (ImGui::BeginTabItem("Main Save & Mario Flags")) {
        if (IS_RANDO(gCurrSaveFileNum - 1)) {
            ImGui::SeparatorText("Rando Save Loaded, use the Rando Tab to make changes");
        }
        ImGui::BeginDisabled(IS_RANDO(gCurrSaveFileNum - 1));
        ImGui::Text("Mario Flags");
        DrawFlagTableArray32(flagTables[1], 0, gMarioState->flags);
        ImGui::Text("Save File Flags");
        DrawFlagTableArray32(flagTables[0], 0, gSaveBuffer.files[gCurrSaveFileNum - 1][0].flags);
        ImGui::SeparatorText("Positionals");
        std::string posX = std::to_string(gMarioState->pos[0]);
        std::string posY = std::to_string(gMarioState->pos[1]);
        std::string posZ = std::to_string(gMarioState->pos[2]);
        ImGui::Text("Pos X:");
        ImGui::SameLine();
        ImGui::Text(posX.c_str());
        
        ImGui::Text("Pos Y:");
        ImGui::SameLine();
        ImGui::Text(posY.c_str());

        ImGui::Text("Pos Z:");
        ImGui::SameLine();
        ImGui::Text(posZ.c_str());

        ImGui::EndDisabled();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Course Stars & Coins")) {
        if (IS_RANDO(gCurrSaveFileNum - 1)) {
            ImGui::SeparatorText("Rando Save Loaded, use the Rando Tab to make changes");
        }
        ImGui::BeginDisabled(IS_RANDO(gCurrSaveFileNum - 1));
        for (int i = 0; i < COURSE_COUNT; i++) {
            ImGui::PushID(i);
            ImGui::Text("%s", courseNames[i]);
            if (ImGui::BeginTable("Course Stars", 8, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableNextColumn();
                for (int s = 0; s < 8; s++) {
                    if (s <= 6) {
                        std::string labelStr = "##courseStars" + std::to_string(s);
                        const char* label = labelStr.c_str();
                        bool isChecked = gSaveBuffer.files[gCurrSaveFileNum - 1][0].courseStars[i] & (1 << s);

                        UIWidgets::PushStyleCheckbox(WIDGET_COLOR);
                        if (UIWidgets::Checkbox(label, &isChecked)) {
                            ModifyStarFlags(isChecked, i, s, gCurrSaveFileNum - 1);
                        }
                        UIWidgets::PopStyleCheckbox();
                    } else {
                        std::string labelStr2 = "##courseCoins" + std::to_string(s);
                        const char* label2 = labelStr2.c_str();
                        int32_t coinCount = gSaveBuffer.files[gCurrSaveFileNum - 1][0].courseCoinScores[i];

                        UIWidgets::PushStyleInput(WIDGET_COLOR);
                        if (UIWidgets::InputInt(label2, &coinCount,
                                                UIWidgets::InputOptions{}
                                                    .Size(ImVec2(50.0f, 0))
                                                    .LabelPosition(UIWidgets::LabelPositions::None))) {}
                        UIWidgets::PopStyleInput();
                    }
                    ImGui::TableNextColumn();
                }
                ImGui::EndTable();
            }
            ImGui::PopID();
        }
        ImGui::EndDisabled();
        ImGui::EndTabItem();
    }

    if (!Rando::Logic::shuffledPool.empty()) {
        if (ImGui::BeginTabItem("Rando")) {
            if (ImGui::BeginChild("RandoChild")) {
                if (ImGui::BeginTabBar("RandoTabBar")) {
                    if (ImGui::BeginTabItem("Check Editor")) {
                        if (ImGui::BeginChild("RandoEditorChild")) {
                            if (ImGui::BeginTable("Rando Save Editor", 4, ImGuiTableFlags_SizingFixedFit)) {
                                ImGui::TableSetupColumn("Obtained", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                                ImGui::TableSetupColumn("Check Name");
                                ImGui::TableSetupColumn("Item Name");
                                ImGui::TableSetupColumn("Course Num", ImGuiTableColumnFlags_WidthFixed, 32.0f);

                                ImGui::TableNextColumn();
                                for (auto& entry : Rando::Logic::shuffledPool) {
                                    if (entry.randoCheckId == RC_UNKNOWN) {
                                        continue;
                                    }
                                    Rando::StaticData::RandoStaticCheck randoStaticCheck =
                                        Rando::StaticData::Checks[entry.randoCheckId];
                                    const char* texture = entry.randoItemId == RI_STAR       ? texture_hud_char_star
                                                          : entry.randoItemId == RI_COIN_RED ? "Red Coin Icon"
                                                                                             : "Blue Coin Icon";

                                    ImGui::PushID(entry.randoCheckId);
                                    if (UIWidgets::Checkbox("##obtained", &entry.obtained)) {
                                        bool toggleTo = entry.obtained;
                                        int16_t courseNumber = Ship_GetCourseByLevel(randoStaticCheck.levelId);

                                        RANDO_SAVE_CHECKS(selectedFileNum)[entry.randoCheckId].obtained = toggleTo;

                                        if (entry.randoItemId == RI_STAR) {
                                            ModifyStarFlags(toggleTo, courseNumber, entry.randoAct, selectedFileNum);
                                        } else if ((entry.randoItemId == RI_COIN_BLUE ||
                                                    entry.randoItemId == RI_COIN_RED) &&
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
                                        RandoSaveFile();

                                        Notification::Emit(
                                            { .itemIcon = texture,
                                              .message = Rando::StaticData::Items[entry.randoItemId].name,
                                              .suffix = toggleTo ? "obtained." : "removed." });
                                    }

                                    ImGui::TableNextColumn();
                                    ImGui::TextColored(entry.obtained
                                                           ? UIWidgets::ColorValues.at(UIWidgets::Colors::Green)
                                                           : UIWidgets::ColorValues.at(UIWidgets::Colors::White),
                                                       Rando::StaticData::Checks[entry.randoCheckId].name);

                                    ImGui::TableNextColumn();
                                    if (ImGui::ImageButton(
                                            randoStaticCheck.name,
                                            Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(
                                                texture),
                                            ImVec2(32.0f, 32.0f))) {
                                        popUpId = randoStaticCheck.randoCheckId;
                                        shouldPopUpOpen = true;
                                        ImGui::OpenPopup("ObjectSubMenu");
                                    }
                                    HandlePopUpContext(popUpId);

                                    ImGui::TableNextColumn();
                                    if (entry.randoItemId == RI_STAR) {
                                        if (ImGui::ImageButton(
                                                std::to_string(entry.randoAct).c_str(),
                                                Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(
                                                    digitList[entry.randoAct + 1]),
                                                ImVec2(32.0f, 32.0f))) {
                                            popUpId = randoStaticCheck.randoCheckId;
                                            shouldPopUpOpen = true;
                                            ImGui::OpenPopup("ActSubMenu");
                                        }
                                    }
                                    ImGui::PopID();
                                    ImGui::TableNextColumn();
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndChild();
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Flag Editor")) {
                        for (auto& [label, flag] : randoFlagList) {
                            bool isUnlocked = (save_file_get_flags() & flag) != 0;
                            if (UIWidgets::Checkbox(label.c_str(), &isUnlocked)) {
                                if (isUnlocked) {
                                    gSaveBuffer.files[selectedFileNum][0].flags |= flag;
                                } else {
                                    gSaveBuffer.files[selectedFileNum][0].flags &= ~flag;
                                }
                                RandoSaveFile();
                            }
                        }
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    UIWidgets::PopStyleTabs();
    ImGui::PopStyleColor(3);
}
