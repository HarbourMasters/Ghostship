#include "ModManager.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <imgui.h>
#include <ship/resource/archive/Archive.h>
#include <ship/window/gui/IconsFontAwesome4.h>

#include "GhostshipGui.hpp"
#include "UIWidgets.hpp"
#include "port/ModList.h"
#include "port/ModRules.h"
#include "port/ShipCompat.h"
#include "port/romhack/Romhack.h"
#include "RomhackMenu.h"

namespace {

bool sEditing = false;
// What the lists show while editing: enabled mods from the one that wins down, and the disabled ones by name.
std::vector<std::string> sEnabled;
std::vector<std::string> sDisabled;
int sDragSource = -1;
int sDragTarget = -1;
// The base game: "" for Super Mario 64, or an installed romhack's stem.
std::string sBase;

// A romhack in the base game list: installed (it has a stem), supported (it has a recipe), or both.
struct Hack {
    std::string stem;
    std::string name;
    std::string version;
    std::string website;
    const Romhack::Supported* recipe = nullptr;
};
std::vector<Hack> sHacks;
bool sHacksLoaded = false;

std::string Label(const Hack& hack) {
    return hack.version.empty() ? hack.name : hack.name + " (v" + hack.version + ")";
}

void RefreshHacks() {
    sHacks.clear();
    const auto& supported = RomhackMenu::Supported();
    std::vector<bool> installed(supported.size(), false);
    for (const std::string& stem : Romhack::ListArchives()) {
        Hack hack{ stem, stem + ".o2r" };
        if (const auto info = Romhack::ReadArchiveInfo(stem)) {
            hack.name = info->name.empty() ? hack.name : info->name;
            hack.version = info->version;
            hack.website = info->website;
            for (size_t i = 0; i < supported.size(); i++) {
                if (!installed[i] && (supported[i].id == info->id || supported[i].name == info->name)) {
                    installed[i] = true;
                    hack.recipe = &supported[i];
                }
            }
        }
        sHacks.push_back(std::move(hack));
    }
    for (size_t i = 0; i < supported.size(); i++) {
        if (!installed[i]) {
            sHacks.push_back({ "", supported[i].name, supported[i].version, supported[i].website, &supported[i] });
        }
    }
    std::sort(sHacks.begin(), sHacks.end(), [](const Hack& a, const Hack& b) { return a.name < b.name; });
    sHacksLoaded = true;
}

const ModList::Entry* Find(const std::string& name) {
    for (const ModList::Entry& entry : ModList::Entries()) {
        if (entry.archive->GetManifest().Name == name) {
            return &entry;
        }
    }
    return nullptr;
}

void SortDisabled() {
    std::sort(sDisabled.begin(), sDisabled.end(), [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                            [](char x, char y) { return std::tolower(x) < std::tolower(y); });
    });
}

// The lists as they are saved: ModList keeps its entries from the lowest priority up, so the last one wins.
void Reset() {
    sBase = Romhack::GetSelected();
    if (std::none_of(sHacks.begin(), sHacks.end(), [](const Hack& hack) { return !hack.stem.empty() && hack.stem == sBase; })) {
        sBase.clear();
    }
    sEnabled.clear();
    sDisabled.clear();
    const auto& entries = ModList::Entries();
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        (it->enabled ? sEnabled : sDisabled).push_back(it->archive->GetManifest().Name);
    }
    SortDisabled();
}

void QuitToApply() {
    GhostshipGui::RegisterPopup("Restart Required",
                                "The base game and mods load when Ghostship starts, so the changes apply next\n"
                                "time. Quit Ghostship now?",
                                "Quit", "Later", []() { ShipCompat::GetWindow()->Close(); });
}

std::string BasesText(const ModRules::Rules& rules) {
    std::string text;
    for (const ModRules::BaseRef& ref : *rules.worksWith) {
        std::string name = ref.name == ModRules::kBaseVanilla      ? "Super Mario 64"
                           : ref.name == ModRules::kBaseAnyRomhack ? "any romhack"
                           : ref.name == "*"                       ? "anything"
                                                                   : ref.name;
        text += (text.empty() ? "" : ", ") + name + (ref.versions.empty() ? "" : " " + ref.versions);
    }
    return text;
}

// What kind of mod it is, in front of its name: one fixed-width slot per kind, as many as the mod with the most kinds
// needs, so the names line up.
void DrawKinds(const std::string& name) {
    struct Kind {
        bool shown;
        const char* glyph;
        UIWidgets::Colors color;
        const char* tooltip;
    };
    const ModList::Entry* entry = Find(name);
    if (entry == nullptr) {
        return;
    }
    const Kind kinds[] = {
#ifdef ENABLE_SCRIPTING
        { entry->code, ICON_FA_CODE, UIWidgets::Colors::Cyan, "Code mod: runs C code the game compiles when it starts." },
#else
        { entry->code, ICON_FA_CODE, UIWidgets::Colors::Cyan,
          "Code mod: its code doesn't run on this platform, only its files are used." },
#endif
        { entry->shaders, ICON_FA_PAINT_BRUSH, UIWidgets::Colors::Orange,
          "Shader mod: adds post-processing passes or material shaders. Passes can be turned on and off in "
          "Shaders > Shader Packs." },
    };
    int slots = 0;
    for (const ModList::Entry& other : ModList::Entries()) {
        slots = std::max(slots, (int)other.code + (int)other.shaders);
    }
    if (slots == 0) {
        return;
    }
    float slot = 0.0f;
    for (const Kind& kind : kinds) {
        slot = std::max(slot, ImGui::CalcTextSize(kind.glyph).x);
    }
    const float gap = ImGui::GetStyle().ItemInnerSpacing.x;

    ImGui::SameLine();
    const float start = ImGui::GetCursorPosX();
    int drawn = 0;
    for (const Kind& kind : kinds) {
        if (!kind.shown) {
            continue;
        }
        ImGui::SetCursorPosX(start + drawn * (slot + gap) + (slot - ImGui::CalcTextSize(kind.glyph).x) / 2.0f);
        ImGui::TextColored(UIWidgets::ColorValues.at(kind.color), "%s", kind.glyph);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", kind.tooltip);
        }
        ImGui::SameLine(0.0f, 0.0f);
        drawn++;
    }
    ImGui::SetCursorPosX(start + slots * slot + (slots - 1) * gap);
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
}

// What the mod needs, after its name: the bases it's limited to, and why it didn't load or how its code built.
void DrawIndicators(const std::string& name) {
    const ModList::Entry* entry = Find(name);
    if (entry == nullptr) {
        return;
    }
    if (entry->rules.worksWith.has_value() && !entry->rules.worksWith->empty()) {
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Purple), ICON_FA_GAMEPAD);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Only loads with %s.", BasesText(entry->rules).c_str());
        }
    }
    if (entry->enabled && !entry->mounted && !entry->status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), ICON_FA_EXCLAMATION_TRIANGLE);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", entry->status.c_str());
        }
    } else if (entry->mounted && !entry->status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(entry->failed ? UIWidgets::Colors::Red : UIWidgets::Colors::Green),
                           "%s", entry->failed ? ICON_FA_EXCLAMATION_TRIANGLE : ICON_FA_CHECK);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", entry->status.c_str());
        }
    }
}

void HandleDragAndDrop(int index, const std::string& name) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("ModMove", &index, sizeof(int));
        ImGui::Text("Move %s", name.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ModMove")) {
            sDragSource = *static_cast<const int*>(payload->Data);
            sDragTarget = index;
        }
        ImGui::EndDragDropTarget();
    }
}

void DrawList(bool enabled) {
    std::vector<std::string>& list = enabled ? sEnabled : sDisabled;
    const auto buttonOptions = UIWidgets::ButtonOptions().Color(THEME_COLOR);
    std::string move;
    int swapFrom = -1;
    int swapTo = -1;

    for (int i = 0; i < static_cast<int>(list.size()); i++) {
        const std::string name = list[i];
        ImGui::PushID(name.c_str());
        if (enabled) {
            ImGui::BeginGroup();
        }
        if (UIWidgets::StateButton("move", enabled ? ICON_FA_ARROW_RIGHT : ICON_FA_ARROW_LEFT, ImVec2(25, 25),
                                   buttonOptions)) {
            move = name;
        }
        if (enabled) {
            ImGui::SameLine();
            ImGui::BeginDisabled(i == 0);
            if (UIWidgets::StateButton("up", ICON_FA_ARROW_UP, ImVec2(25, 25), buttonOptions)) {
                swapFrom = i;
                swapTo = i - 1;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(i == static_cast<int>(list.size()) - 1);
            if (UIWidgets::StateButton("down", ICON_FA_ARROW_DOWN, ImVec2(25, 25), buttonOptions)) {
                swapFrom = i;
                swapTo = i + 1;
            }
            ImGui::EndDisabled();
        }
        DrawKinds(name);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", name.c_str());
        DrawIndicators(name);
        if (enabled) {
            ImGui::EndGroup();
            HandleDragAndDrop(i, name);
        }
        ImGui::PopID();
    }

    if (enabled && sDragTarget != -1) {
        const std::string dragged = list[sDragSource];
        list.erase(list.begin() + sDragSource);
        list.insert(list.begin() + sDragTarget, dragged);
        sDragSource = sDragTarget = -1;
    }
    if (swapFrom != -1) {
        std::swap(list[swapFrom], list[swapTo]);
    }
    if (!move.empty()) {
        list.erase(std::find(list.begin(), list.end(), move));
        if (enabled) {
            sDisabled.push_back(move);
            SortDisabled();
        } else {
            sEnabled.push_back(move);
        }
    }
}

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", text);
    }
}

// The website button, at the right edge of the row.
void DrawLink(const Hack& hack) {
    if (hack.website.empty()) {
        return;
    }
    ImGui::SameLine();
    const float width = ImGui::CalcTextSize(ICON_FA_EXTERNAL_LINK).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - width));
    if (ImGui::SmallButton(ICON_FA_EXTERNAL_LINK)) {
        SDL_OpenURL(hack.website.c_str());
    }
    Tooltip(hack.website.c_str());
}

void DrawHack(const Hack& hack) {
    ImGui::PushID(hack.stem.empty() ? hack.name.c_str() : hack.stem.c_str());
    const bool installed = !hack.stem.empty();
    ImGui::BeginDisabled(!sEditing || !installed);
    if (ImGui::RadioButton(Label(hack).c_str(), installed && sBase == hack.stem)) {
        sBase = hack.stem;
    }
    ImGui::EndDisabled();
    if (installed && hack.stem == Romhack::ActiveStem()) {
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Green), "Playing");
    }
    if (!installed) {
        ImGui::SameLine();
        if (hack.recipe->fromBaseRom) {
            ImGui::BeginDisabled(RomhackMenu::Extracting());
            if (ImGui::SmallButton("Generate")) {
                RomhackMenu::Generate(hack.recipe);
            }
            ImGui::EndDisabled();
            Tooltip("Not installed yet. Pick your Super Mario 64 ROM: the hack's patch is applied to it and the "
                    "romhack is generated once.");
        } else {
            ImGui::TextDisabled("Not installed");
            Tooltip("Patch the hack onto your Super Mario 64 ROM yourself, then use Pick ROM below.");
        }
    }
    DrawLink(hack);
    ImGui::PopID();
}

// The base game the mods load on top of: Super Mario 64 or one installed romhack. Hacks this build supports but
// that aren't installed yet are listed too, with a way to generate them.
void DrawBaseGame() {
    ImGui::BeginDisabled(!sEditing);
    if (ImGui::RadioButton("Super Mario 64", sBase.empty())) {
        sBase.clear();
    }
    ImGui::EndDisabled();
    if (Romhack::Active() == nullptr) {
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Green), "Playing");
    }
    for (const Hack& hack : sHacks) {
        DrawHack(hack);
    }

    if (RomhackMenu::Extracting()) {
        const std::string label = "Generating " + RomhackMenu::ExtractingName() + "...";
        ImGui::ProgressBar(RomhackMenu::Progress(), ImVec2(-1.0f, 0.0f), label.c_str());
    } else {
        ImGui::TextDisabled("Have a patched romhack ROM?");
        ImGui::SameLine();
        if (ImGui::SmallButton("Pick ROM...")) {
            RomhackMenu::Generate(nullptr);
        }
        Tooltip("Pick the patched .z64 of a supported romhack. It's generated once, then listed above.");
    }
}

} // namespace

void GhostshipGui::DrawModManager() {
    RomhackMenu::Update();
    if (!sHacksLoaded || RomhackMenu::TakeInstalledChanged()) {
        RefreshHacks();
    }
    if (!sEditing) {
        Reset();
    }

    ImGui::TextWrapped("Pick the game to play and the mods to load on top of it. Changes apply after restarting "
                       "Ghostship.");

    auto editOptions = UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR);
    editOptions.disabled = sEditing || RomhackMenu::Extracting();
    editOptions.disabledTooltip = sEditing ? "Already editing..." : "Wait for the romhack to finish generating.";
    editOptions.tooltip = "Change the base game, and turn mods on, off or reorder them.";
    if (UIWidgets::Button("Edit##mods", editOptions)) {
        sEditing = true;
    }
    if (sEditing) {
        ImGui::SameLine();
        if (UIWidgets::Button("Cancel##mods", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
            sEditing = false;
        }
        ImGui::SameLine();
        auto resetOptions = UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline);
        resetOptions.tooltip = "Super Mario 64 with no mods.";
        if (UIWidgets::Button("Reset##mods", resetOptions)) {
            sBase.clear();
            sDisabled.insert(sDisabled.end(), sEnabled.begin(), sEnabled.end());
            sEnabled.clear();
            SortDisabled();
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Apply##mods",
                              UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Color(THEME_COLOR))) {
            sEditing = false;
            if (sBase != Romhack::GetSelected()) {
                Romhack::SetSelected(sBase);
            }
            ModList::SetOrder(sEnabled, sDisabled);
            QuitToApply();
        }
    }

    ImGui::SeparatorText("Base Game");
    DrawBaseGame();

    ImGui::SeparatorText("Mods");
    ImGui::TextDisabled("The top mod wins over the ones below it. Drag a mod or use the arrows to reorder.");
    ImGui::BeginDisabled(!sEditing);
    if (ImGui::BeginTable("tableMods", 2, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Disabled", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("Enabled Mods", ImVec2(0, 200))) {
            DrawList(true);
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("Disabled Mods", ImVec2(0, 200))) {
            DrawList(false);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    if (ModList::Entries().empty()) {
        ImGui::TextDisabled("No mods yet. Put .o2r mods in the mods folder.");
    }
    ImGui::SeparatorText("Installed Mods");
}
