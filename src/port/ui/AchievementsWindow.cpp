// Local includes
#include "AchievementsWindow.h"

#include <map>
#include <string>

#include "port/mods/achievements/Achievements.h"

// Helper to get category name and color
struct CategoryStyle {
    std::string name;
    ImVec4 color;
};

std::map<AchievementCategory, CategoryStyle> gCatStyles = {
    { AchievementCategory::Stars, { "Stars", ImVec4(1.0f, 0.84f, 0.0f, 1.0f) } },  // Gold
    { AchievementCategory::Caps, { "Caps", ImVec4(0.8f, 0.8f, 0.8f, 1.0f) } },     // Silver
    { AchievementCategory::Bosses, { "Bosses", ImVec4(1.0f, 0.2f, 0.2f, 1.0f) } }, // Red
    { AchievementCategory::Levels, { "Ranks", ImVec4(0.4f, 0.7f, 1.0f, 1.0f) } },  // Blue
    { AchievementCategory::Extras, { "Extras", ImVec4(0.6f, 1.0f, 0.6f, 1.0f) } }, // Green
    { AchievementCategory::Deaths, { "Death", ImVec4(0.5f, 0.5f, 0.5f, 1.0f) } }   // Gray
};

void AchievementsWindow::InitElement() {
    // No initialization needed for now
}

void AchievementsWindow::UpdateElement() {
    // No dynamic updates needed for now
}

void DrawAchievementCard(const Achievement& ach, float cardWidth) {
    const float cardHeight = 100.0f;
    const auto data = Achievement_GetProgress(ach.id);
    const float progress = static_cast<float>(data->progress) / static_cast<float>(ach.maxProgress);

    ImGui::PushID(ach.id.c_str());

    ImGui::BeginChild(ach.id.c_str(), ImVec2(cardWidth, cardHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImGui::Image(Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(ach.icon),
                    ImVec2(cardHeight - 10, cardHeight - 10), ImVec2(0, 0), ImVec2(1.0f, 1.0f));

    ImGui::SameLine();

    ImGui::BeginGroup();

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
    ImGui::Text("%s", ach.name.c_str());
    ImGui::PopStyleColor();

    // Description
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::Text("%s", ach.description.c_str());
    ImGui::PopStyleColor();

    // Progress Bar
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

    char progBuf[32];
    snprintf(progBuf, sizeof(progBuf), "%d / %d", data->progress, ach.maxProgress);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    // 3. This now stretches to the edge of the card, leaving a perfect 10px margin
    ImGui::ProgressBar(progress, ImVec2(-10.0f, 21.0f), progBuf);
    ImGui::PopStyleColor(2);

    ImGui::EndGroup();

    // Satella Score
    ImGui::SameLine();

    float contentRegionMaxX = ImGui::GetWindowContentRegionMax().x;
    float scoreWidth = ImGui::CalcTextSize("1000 G").x + 30.0f;
    ImGui::SetCursorPosX(contentRegionMaxX - scoreWidth);

    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
    ImGui::Text("1000");
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "(G)");
    ImGui::EndGroup();

    ImGui::EndChild();
    ImGui::PopID();
}

void AchievementsWindow::DrawElement() {
    int cols = 2;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float cardWidth = (ImGui::GetContentRegionAvail().x / cols) - (spacing * 0.5f);

    ImGui::BeginGroup();
    for (size_t i = 0; i < gAchievementList.size(); i++) {
        DrawAchievementCard(gAchievementList[i], cardWidth);

        if ((i + 1) % cols != 0 && (i + 1) < gAchievementList.size()) {
            ImGui::SameLine();
        }
    }
    ImGui::EndGroup();
}