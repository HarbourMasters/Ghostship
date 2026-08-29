// Local includes
#include "AchievementsWindow.h"
#include "port/ShipCompat.h"
#include "UIWidgets.hpp"
#include "fast/Fast3dGui.h"

#include <map>
#include <string>

#include "port/mods/achievements/Achievements.h"

// Helper to get category name and color
struct CategoryStyle {
    std::string name;
    ImVec4 color;
};

std::map<AchievementCategory, CategoryStyle> gCatStyles = {
    { AchievementCategory::Stars, { "Stars", VecFromRGBA8(Color_RGBA8{ 255, 214, 0, 255 }) } }, // Gold

    { AchievementCategory::Caps, { "Caps", VecFromRGBA8(Color_RGBA8{ 204, 204, 204, 255 }) } }, // Silver

    { AchievementCategory::Bosses, { "Bosses", VecFromRGBA8(Color_RGBA8{ 255, 51, 51, 255 }) } }, // Red

    { AchievementCategory::Levels, { "Ranks", VecFromRGBA8(Color_RGBA8{ 102, 179, 255, 255 }) } }, // Blue

    { AchievementCategory::Extras, { "Extras", VecFromRGBA8(Color_RGBA8{ 153, 255, 153, 255 }) } }, // Green

    { AchievementCategory::Deaths, { "Death", VecFromRGBA8(Color_RGBA8{ 128, 128, 128, 255 }) } } // Gray
};

void AchievementsWindow::OnInit(const nlohmann::json& initArgs) {
    Ship::GuiWindow::OnInit(initArgs);
    // No initialization needed for now
}

void AchievementsWindow::UpdateElement() {
    // No dynamic updates needed for now
}

void DrawAchievementCard(const std::pair<const std::string, Achievement>& achPair, float cardWidth) {
    const Achievement& ach = achPair.second;
    const float cardHeight = 100.0f;
    const auto data = Achievement_GetProgress(achPair.first);
    const float progress = static_cast<float>(data->progress) / static_cast<float>(ach.maxProgress);

    ImGui::PushID(achPair.first.c_str());

    ImGui::BeginChild(achPair.first.c_str(), ImVec2(cardWidth, cardHeight), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImGui::Image(std::static_pointer_cast<Fast::Fast3dGui>(ShipCompat::GetWindow()->GetGui())
                     ->GetTextureByName(data->achieved ? ach.icon : std::string(ach.icon) + ".locked"),
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

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, /*ImVec4(1.0f, 0.8f, 0.0f, 1.0f)*/
                          gCatStyles[ach.category].color);
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
    std::vector<std::string> sorted;
    for (const auto& achPair : gAchievementList) {
        sorted.push_back(achPair.first);
    }

    std::sort(sorted.begin(), sorted.end(), [](const std::string& a, const std::string& b) {
        return gAchievementList[a].order < gAchievementList[b].order;
    });

    for (const auto& achId : sorted) {
        auto& ach = gAchievementList[achId];

        DrawAchievementCard({ achId, ach }, cardWidth);

        if ((ach.order + 1) % cols != 0 && (ach.order + 1) < gAchievementList.size()) {
            ImGui::SameLine();
        }
    }
    ImGui::EndGroup();
}