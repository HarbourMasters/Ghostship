#include "RomhackMenuWindow.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <thread>

#include <SDL2/SDL.h>
#include <imgui.h>
#include <ship/Context.h>
#include <ship/window/gui/IconsFontAwesome4.h>

#include "GhostshipGui.hpp"
#include "UIWidgets.hpp"
#include "port/GameExtractor.h"
#include "port/ShipCompat.h"
#include "port/romhack/Romhack.h"

namespace {

GameExtractor sExtractor;
std::atomic<bool> sExtracting = false;
std::atomic<size_t> sExtractCount{ 0 }, sExtractTotal{ 0 };
std::string sPendingStem;

enum class ExtractResult { None, Done, Failed };
std::atomic<ExtractResult> sResult = ExtractResult::None;

bool sEditing = false;
std::string sEnabled;
std::vector<Romhack::Supported> sSupported;

void QuitToApply() {
    GhostshipGui::RegisterPopup(
        "Restart Required", "The romhack is switched the next time Ghostship starts.\nQuit Ghostship now?", "Quit",
        "Later", []() { ShipCompat::GetWindow()->Close(); });
}

void SelectAndQuit(const std::string& stem) {
    Romhack::SetSelected(stem);
    QuitToApply();
}

void StartExtraction() {
    sExtractor = GameExtractor();
    if (!sExtractor.SelectGameFromUI()) {
        return;
    }
    const auto recipe = sExtractor.GetRomhack();
    if (!recipe.has_value()) {
        GhostshipGui::RegisterPopup("Not a Supported Romhack",
                                    "That ROM is not a romhack Ghostship has a recipe for.\nPatch the hack onto the "
                                    "ROM it was made for and pick the patched .z64.");
        return;
    }
    sPendingStem = std::filesystem::path(recipe->output).stem().string();
    sExtracting = true;
    std::thread([]() {
        sExtractor.Parse(sExtractTotal);
        // Into the same app directory the mods/ folder is read from, so mods/~romhacks is found at boot.
        const bool ok = sExtractor.GenerateOTRTo(sExtractCount, Ship::Context::GetAppBundlePath(),
                                                 Ship::Context::GetAppDirectoryPath());
        sResult = ok ? ExtractResult::Done : ExtractResult::Failed;
        sExtracting = false;
    }).detach();
}

void ReportExtraction() {
    const ExtractResult result = sResult.exchange(ExtractResult::None);
    if (result == ExtractResult::Failed) {
        const std::string msg = "Extraction failed:\n" + GameExtractor::sLastError;
        GhostshipGui::RegisterPopup("Romhack Extraction Failed", msg.c_str());
    } else if (result == ExtractResult::Done) {
        const std::string stem = sPendingStem;
        GhostshipGui::RegisterPopup(
            "Romhack Ready", "Play it now? Ghostship needs a restart to load the romhack.", "Play", "Later",
            [stem]() { SelectAndQuit(stem); },
            [stem]() {
                sEnabled = Romhack::GetSelected();
            });
        sEditing = false;
    }
}

void DrawList(bool enabled, const std::vector<std::string>& stems) {
    for (const auto& stem : stems) {
        if ((stem == sEnabled) != enabled) {
            continue;
        }
        ImGui::PushID(stem.c_str());
        if (UIWidgets::Button(enabled ? ICON_FA_ARROW_RIGHT : ICON_FA_ARROW_LEFT,
                              UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR))) {
            sEnabled = enabled ? "" : stem;
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s.o2r", stem.c_str());
        ImGui::PopID();
    }
}

void DrawInstalled() {
    const std::vector<std::string> stems = Romhack::ListArchives();
    if (!sEnabled.empty() && std::find(stems.begin(), stems.end(), sEnabled) == stems.end()) {
        sEnabled.clear();
    }

    auto editOptions = UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR);
    editOptions.disabled = sEditing || sExtracting;
    editOptions.disabledTooltip = "Already editing...";
    if (UIWidgets::Button("Edit##romhacks", editOptions)) {
        sEditing = true;
    }
    if (sEditing) {
        ImGui::SameLine();
        if (UIWidgets::Button("Cancel##romhacks", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
            sEditing = false;
            sEnabled = Romhack::GetSelected();
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Clear List##romhacks", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
            sEnabled.clear();
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Apply##romhacks",
                              UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR))) {
            sEditing = false;
            if (sEnabled != Romhack::GetSelected()) {
                SelectAndQuit(sEnabled);
            }
        }
    }

    ImGui::BeginDisabled(!sEditing);
    if (ImGui::BeginTable("tableRomhacks", 2, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Enabled Romhack", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Disabled Romhacks", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("Enabled Romhack", ImVec2(0, 160))) {
            DrawList(true, stems);
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("Disabled Romhacks", ImVec2(0, 160))) {
            DrawList(false, stems);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    if (stems.empty()) {
        ImGui::TextDisabled("No romhacks yet. Generate one from a patched ROM above.");
    }
}

void DrawSupported() {
    if (sSupported.empty()) {
        ImGui::TextDisabled("No romhacks are supported by this build yet.");
        return;
    }
    for (const auto& hack : sSupported) {
        ImGui::PushID(hack.id.c_str());
        auto linkOptions = UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR);
        linkOptions.tooltip = hack.website.c_str();
        linkOptions.disabled = hack.website.empty();
        if (UIWidgets::Button(ICON_FA_EXTERNAL_LINK, linkOptions)) {
            SDL_OpenURL(hack.website.c_str());
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        if (hack.version.empty()) {
            ImGui::Text("%s", hack.name.c_str());
        } else {
            ImGui::Text("%s (v%s)", hack.name.c_str(), hack.version.c_str());
        }
        ImGui::PopID();
    }
}

} // namespace

void RomhackMenuWindow::OnInit(const nlohmann::json& initArgs) {
    Ship::GuiWindow::OnInit(initArgs);
    sEnabled = Romhack::GetSelected();
    sSupported = Romhack::ListSupported();
}

void RomhackMenuWindow::DrawElement() {
    ReportExtraction();

    if (const auto* active = Romhack::Active()) {
        if (active->version.empty()) {
            ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Yellow), "Playing: %s",
                               active->name.c_str());
        } else {
            ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Yellow), "Playing: %s (v%s)",
                               active->name.c_str(), active->version.c_str());
        }
    } else {
        ImGui::TextDisabled("Playing: Super Mario 64");
    }

    auto generateOptions = UIWidgets::ButtonOptions().Color(THEME_COLOR);
    generateOptions.tooltip = "Pick a patched romhack ROM. Its changed assets are extracted to mods/~romhacks once.";
    generateOptions.disabled = sExtracting;
    if (UIWidgets::Button("Generate Romhack from ROM", generateOptions)) {
        StartExtraction();
    }
    if (sExtracting) {
        const size_t total = sExtractTotal;
        const float progress = total > 0 ? std::min<float>(1.0f, (float)sExtractCount / (float)total) : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Extracting romhack...");
    }

    ImGui::SeparatorText("Installed Romhacks");
    DrawInstalled();

    ImGui::SeparatorText("Supported Romhacks");
    DrawSupported();
}
