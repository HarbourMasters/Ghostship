#include "EventDebugger.h"

#include <string>
#include <version>

#include "port/Engine.h"
#include "port/ui/UIWidgets.hpp"
#include "port/ui/cvar_prefixes.h"
#include <ship/utils/StringHelper.h>
#include "port/hooks/impl/EventSystem.h"

#define THEME_COLOR UIWidgets::Colors::Orange

static bool hookOptCollapseAll; // A bool that will collapse all hook group once
static bool hookOptExpandAll;   // A bool that will expand all hook group once

const ImVec4 grey = ImVec4(0.75, 0.75, 0.75, 1);
const ImVec4 yellow = ImVec4(1, 1, 0, 1);
const ImVec4 red = ImVec4(1, 0, 0, 1);

void DrawEventCallerInfo(std::string& name, EventRegistration& registry) {
    ImGui::Text("Total Callers Registered: %d", registry.callers.size());

    if (ImGui::BeginTable(("Table##" + std::string(name)).c_str(), 4,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
                              ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Registration Info", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("# Calls", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        int i = 0;
        for (auto& [_, caller] : registry.callers) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%d", i++);

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s:%d ", caller.path, caller.line);

            ImGui::TableNextColumn();
            ImGui::Text("%llu", caller.count);
        }
        ImGui::EndTable();
    }
}

void DrawEventListenerInfo(std::string& name, EventRegistration& registry) {
    ImGui::Text("Total Listeners Registered: %d", registry.listeners.size());

    if (ImGui::BeginTable(("Table##" + std::string(name)).c_str(), 4,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
                              ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Listener Info", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        int i = 0;
        for (auto& listener : registry.listeners) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%d", i++);

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s:%d ", listener.metadata.path, listener.metadata.line);

            ImGui::TableNextColumn();
            switch (listener.priority) {
                case EVENT_PRIORITY_LOW:
                    ImGui::TextColored(grey, "Low");
                    break;
                case EVENT_PRIORITY_NORMAL:
                    ImGui::TextColored(yellow, "Normal");
                    break;
                case EVENT_PRIORITY_HIGH:
                    ImGui::TextColored(red, "High");
                    break;
            }
        }
        ImGui::EndTable();
    }
}

void EventDebuggerWindow::DrawElement() {
    bool collapseLogic = false;
    auto events = EventSystem::Instance->GetEventRegistrations();
    bool doingCollapseOrExpand = hookOptExpandAll || hookOptCollapseAll;

    ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0));

    if (UIWidgets::Button("Expand All", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(UIWidgets::Sizes::Inline))) {
        hookOptCollapseAll = false;
        hookOptExpandAll = true;
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Collapse All",
                          UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(UIWidgets::Sizes::Inline))) {
        hookOptExpandAll = false;
        hookOptCollapseAll = true;
    }

    ImGui::PushFont(GameEngine::Instance->fontMonoLarger);

    for (auto& [id, registry] : events) {
        auto name = StringHelper::Sprintf("%s (ID: %d) [%d]", registry.name, id, registry.listeners.size());

        if (doingCollapseOrExpand) {
            if (hookOptExpandAll) {
                collapseLogic = true;
            } else if (hookOptCollapseAll) {
                collapseLogic = false;
            }
            ImGui::SetNextItemOpen(collapseLogic, ImGuiCond_Always);
        }

        if (ImGui::TreeNode(name.c_str())) {
            DrawEventCallerInfo(name, registry);
            DrawEventListenerInfo(name, registry);
            ImGui::TreePop();
        }
    }

    ImGui::PopFont();
    ImGui::EndDisabled();

    if (doingCollapseOrExpand) {
        hookOptExpandAll = false;
        hookOptCollapseAll = false;
    }
}

void EventDebuggerWindow::InitElement() {
    hookOptExpandAll = false;
    hookOptCollapseAll = false;
}
