#include "RomhackMenu.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <thread>
#include <utility>

#include <imgui.h>
#include <ship/Context.h>

#include "GhostshipGui.hpp"
#include "port/GameExtractor.h"
#include "port/ShipCompat.h"
#include "port/console/LogFeed.h"

namespace {

GameExtractor sExtractor;
std::atomic<bool> sExtracting = false;
std::atomic<size_t> sExtractCount{ 0 }, sExtractTotal{ 0 };
std::string sPendingStem;
std::string sPendingName;

enum class ExtractResult { None, Done, Failed };
std::atomic<ExtractResult> sResult = ExtractResult::None;

bool sLoaded = false;
bool sInstalledChanged = false;
std::vector<Romhack::Supported> sSupported;

// The last warnings and errors an extraction logged, one per line, for its popup.
std::string ProblemsText(size_t max) {
    std::string text;
    for (const auto& line : LogFeed::CapturedProblems(max)) {
        std::string shown = line.text;
        if (shown.size() > 110) {
            shown = shown.substr(0, 107) + "...";
        }
        text += "\n- " + shown;
    }
    return text;
}

void SelectAndQuit(const std::string& stem) {
    Romhack::SetSelected(stem);
    GhostshipGui::RegisterPopup(
        "Restart Required", "The romhack is switched the next time Ghostship starts.\nQuit Ghostship now?", "Quit",
        "Later", []() { ShipCompat::GetWindow()->Close(); });
}

} // namespace

const std::vector<Romhack::Supported>& RomhackMenu::Supported() {
    if (!sLoaded) {
        sSupported = Romhack::ListSupported();
        sLoaded = true;
    }
    return sSupported;
}

void RomhackMenu::Generate(const Romhack::Supported* hack) {
    if (sExtracting) {
        return;
    }
    sExtractor = GameExtractor();
    if (hack != nullptr) {
        sExtractor.SetRequestedRecipe(hack->package);
    }
    if (!sExtractor.SelectGameFromUI()) {
        return;
    }
    const auto recipe = sExtractor.GetRomhack();
    if (!recipe.has_value()) {
        if (hack != nullptr) {
            const std::string msg = "That ROM isn't the one " + hack->name +
                                    " was made for.\nPick your Super Mario 64 ROM, or the hack's patched .z64.";
            GhostshipGui::RegisterPopup("Wrong ROM", msg.c_str());
        } else {
            GhostshipGui::RegisterPopup("Not a Supported Romhack",
                                        "That ROM is not a romhack Ghostship has a recipe for.\nPatch the hack onto "
                                        "the ROM it was made for and pick the patched .z64, or use Generate next to "
                                        "the hack in the list.");
        }
        return;
    }
    sPendingStem = std::filesystem::path(recipe->output).stem().string();
    sPendingName = recipe->info.name.empty() ? sPendingStem : recipe->info.name;
    sExtractCount = 0;
    sExtractTotal = 0;
    LogFeed::BeginCapture();
    sExtracting = true;
    std::thread([]() {
        sExtractor.Parse(sExtractTotal);
        // Into the same app directory the mods/ folder is read from, so mods/~romhacks is found at boot.
        const bool ok = sExtractor.GenerateOTRTo(sExtractCount, Ship::Context::GetAppBundlePath(),
                                                 Ship::Context::GetAppDirectoryPath());
        GameExtractor::ReleaseTorch();
        sResult = ok ? ExtractResult::Done : ExtractResult::Failed;
        sExtracting = false;
    }).detach();
}

void RomhackMenu::Update() {
    const ExtractResult result = sResult.exchange(ExtractResult::None);
    if (result == ExtractResult::None) {
        return;
    }
    LogFeed::EndCapture();
    if (result == ExtractResult::Failed) {
        std::string msg = "Generating " + sPendingName + " failed:\n" + GameExtractor::sLastError;
        const std::string problems = ProblemsText(5);
        if (!problems.empty()) {
            msg += "\n\nWhat it logged last:" + problems;
        }
        GhostshipGui::RegisterPopup("Romhack Extraction Failed", msg.c_str(), "Open Logs", "Close",
                                    []() { GhostshipGui::OpenLogs(spdlog::level::warn); });
    } else {
        sInstalledChanged = true;
        const std::string stem = sPendingStem;
        std::string msg = sPendingName + " is ready.\nPlay it now? Ghostship needs a restart to load the romhack.";
        const size_t warnings = LogFeed::CapturedProblems(SIZE_MAX).size();
        if (warnings > 0) {
            msg += "\n\nIt logged " + std::to_string(warnings) + (warnings == 1 ? " warning" : " warnings") +
                   ", listed in Dev Tools > Console > Logs.";
        }
        GhostshipGui::RegisterPopup("Romhack Ready", msg.c_str(), "Play", "Later", [stem]() { SelectAndQuit(stem); });
    }
}

void RomhackMenu::DrawProgress() {
    if (!sExtracting) {
        return;
    }
    const std::string label = "Generating " + sPendingName + "...";
    ImGui::ProgressBar(Progress(), ImVec2(-1.0f, 0.0f), label.c_str());

    const std::string activity = LogFeed::CurrentActivity();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(activity.empty() ? "Reading the ROM..." : activity.c_str());
    ImGui::PopStyleColor();

    const auto problems = LogFeed::CapturedProblems(50);
    if (problems.empty()) {
        return;
    }
    const float lines = (float)std::min<size_t>(problems.size(), 6);
    if (ImGui::BeginChild("##extractlog", ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * lines + 8.0f),
                          ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
        const bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
        for (const auto& line : problems) {
            const ImVec4 color = line.level >= spdlog::level::err ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                                                  : ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
            ImGui::TextColored(color, "%s", line.text.c_str());
        }
        if (atBottom) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

bool RomhackMenu::Extracting() {
    return sExtracting;
}

float RomhackMenu::Progress() {
    const size_t total = sExtractTotal;
    return total > 0 ? std::min<float>(1.0f, (float)sExtractCount / (float)total) : 0.0f;
}

bool RomhackMenu::TakeInstalledChanged() {
    return std::exchange(sInstalledChanged, false);
}
