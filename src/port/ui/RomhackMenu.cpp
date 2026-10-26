#include "RomhackMenu.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <thread>
#include <utility>

#include <ship/Context.h>

#include "GhostshipGui.hpp"
#include "port/GameExtractor.h"
#include "port/ShipCompat.h"

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

void RomhackMenu::Update() {
    const ExtractResult result = sResult.exchange(ExtractResult::None);
    if (result == ExtractResult::Failed) {
        const std::string msg = "Generating " + sPendingName + " failed:\n" + GameExtractor::sLastError;
        GhostshipGui::RegisterPopup("Romhack Extraction Failed", msg.c_str());
    } else if (result == ExtractResult::Done) {
        sInstalledChanged = true;
        const std::string stem = sPendingStem;
        const std::string msg =
            sPendingName + " is ready.\nPlay it now? Ghostship needs a restart to load the romhack.";
        GhostshipGui::RegisterPopup("Romhack Ready", msg.c_str(), "Play", "Later", [stem]() { SelectAndQuit(stem); });
    }
}

bool RomhackMenu::Extracting() {
    return sExtracting;
}

const std::string& RomhackMenu::ExtractingName() {
    return sPendingName;
}

float RomhackMenu::Progress() {
    const size_t total = sExtractTotal;
    return total > 0 ? std::min<float>(1.0f, (float)sExtractCount / (float)total) : 0.0f;
}

bool RomhackMenu::TakeInstalledChanged() {
    return std::exchange(sInstalledChanged, false);
}
