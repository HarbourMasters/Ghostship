#include "ObjectViewer.h"

#ifndef __APPLE__
#include <windows.h>
#include <dbghelp.h>
#else
#include <dlfcn.h>
#endif

#include "types.h"
#include "sm64.h"
#include "game/camera.h"
#include "engine/math_util.h"
#include "game/level_update.h"
#include "game/object_list_processor.h"

std::unordered_map<uintptr_t, std::string> functionNameCache;
extern "C" struct CameraFOVStatus sFOVState;

void ObjectViewer::InitElement() {
#ifndef __APPLE__
    HANDLE hProcess = GetCurrentProcess();
    SymSetOptions(SYMOPT_NO_IMAGE_SEARCH | SYMOPT_IGNORE_IMAGEDIR);
    SymInitialize(hProcess, "debug", true);
#endif
}

const char* GetFunctionName(const uintptr_t addr) {
    if (functionNameCache.contains(addr)) {
        return functionNameCache[addr].c_str();
    }

#ifndef __APPLE__
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(buffer);

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    if (SymFromAddr(GetCurrentProcess(), addr, &displacement, pSymbol)) {
        functionNameCache[addr] = pSymbol->Name;
        return functionNameCache[addr].c_str(); 
    }
#else
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(addr), &info) && info.dli_sname) {
        functionNameCache[addr] = info.dli_sname;
        return info.dli_sname;
    }
#endif
    // Fallback
    const auto hexString = new char[20];
    snprintf(hexString, 20, "0x%lX", addr);
    functionNameCache[addr] = hexString;
    delete[] hexString;

    return functionNameCache[addr].c_str();
}

void ObjectViewer::UpdateElement() {
}

void ObjectViewer::DrawElement() {
    static char searchBehavior[128] = "";
    static bool filterModelId = false;
    static int searchModelId = 0;
    static bool filterDistance = false;
    static float distanceRange = 2000.0f;

    auto strContainsCaseInsensitive = [](const char* haystack, const char* needle) -> bool {
        if (!haystack || !needle)
            return false;
        if (needle[0] == '\0')
            return true;
        for (int i = 0; haystack[i] != '\0'; i++) {
            int j = 0;
            while (needle[j] != '\0' && tolower(haystack[i + j]) == tolower(needle[j])) {
                j++;
            }
            if (needle[j] == '\0')
                return true;
        }
        return false;
    };

    auto passesFilters = [&](Object* obj) -> bool {
        if (filterModelId && obj->modelId != searchModelId)
            return false;

        if (searchBehavior[0] != '\0') {
            const char* bhvName = GetFunctionName(reinterpret_cast<uintptr_t>(obj->behavior));
            if (!strContainsCaseInsensitive(bhvName, searchBehavior))
                return false;
        }

        if (filterDistance) {
            float dx = obj->oPosX - gMarioState->pos[0];
            float dy = obj->oPosY - gMarioState->pos[1];
            float dz = obj->oPosZ - gMarioState->pos[2];
            if (sqrtf(dx * dx + dy * dy + dz * dz) > distanceRange)
                return false;
        }
        return true;
    };

    if (ImGui::CollapsingHeader("Filters & Utilities", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));

        ImGui::TextDisabled("Behavior Search:");
        ImGui::InputText("##searchBhv", searchBehavior, IM_ARRAYSIZE(searchBehavior));

        ImGui::Spacing();
        ImGui::Checkbox("Filter by Model ID", &filterModelId);
        if (filterModelId) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt("##searchModelId", &searchModelId);
        }

        ImGui::Checkbox("Only list objects near Mario", &filterDistance);
        if (filterDistance) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::DragFloat("Range##distRange", &distanceRange, 10.0f, 0.0f, 30000.0f, "%.0f units");
        }

        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::Spacing();

    auto drawObjectUI = [](Object* obj) {
        bool showHitbox = (obj->header.gfx.node.flags & GRAPH_RENDER_DRAW_DEBUG) != 0;

        if (ImGui::Checkbox("Show 3D Hitbox", &showHitbox)) {
            if (showHitbox)
                obj->header.gfx.node.flags |= GRAPH_RENDER_DRAW_DEBUG;
            else
                obj->header.gfx.node.flags &= ~GRAPH_RENDER_DRAW_DEBUG;
        }

        // OTRTODO: Implement this later
        // if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        // obj->header.gfx.node.flags |= GRAPH_RENDER_DRAW_DEBUG;
        // }

        ImGui::SameLine();
        if (ImGui::Button("Despawn")) {
            obj->activeFlags = ACTIVE_FLAG_DEACTIVATED;
        }

        ImGui::SameLine();
        if (ImGui::Button("Teleport")) {
            obj->oPosX = gMarioState->pos[0];
            obj->oPosY = gMarioState->pos[1] + 200.0f;
            obj->oPosZ = gMarioState->pos[2];
        }

        ImGui::SameLine();
        if (ImGui::Button("Visibility")) {
            obj->header.gfx.node.flags ^= GRAPH_RENDER_INVISIBLE;
        }

        ImGui::SameLine();
        if (ImGui::Button("Copy Addr")) {
            char addrStr[32];
            snprintf(addrStr, sizeof(addrStr), "%p", obj);
            ImGui::SetClipboardText(addrStr);
        }

        ImGui::Spacing();

        if (ImGui::BeginTabBar("ObjectTabs")) {

            // TAB 1: BASIC PROPERTIES
            if (ImGui::BeginTabItem("Basic Properties")) {
                if (ImGui::BeginTable("ObjProps", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Address");
                    ImGui::TableNextColumn();
                    ImGui::Text("%p", obj);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Model ID");
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(150.0f);
                    ImGui::Text("Model: %d", obj->modelId);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Behavior");
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s",
                                       GetFunctionName(reinterpret_cast<uintptr_t>(obj->behavior)));

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Position");
                    ImGui::TableNextColumn();
                    float pos[3] = { obj->oPosX, obj->oPosY, obj->oPosZ };
                    ImGui::SetNextItemWidth(250.0f);
                    if (ImGui::DragFloat3("##pos", pos, 1.0f)) {
                        obj->oPosX = pos[0];
                        obj->oPosY = pos[1];
                        obj->oPosZ = pos[2];
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Flags")) {
                uint32_t activeFlags32 = static_cast<uint32_t>(obj->activeFlags);
                uint32_t gfxFlags32 = static_cast<uint32_t>(obj->header.gfx.node.flags);

                if (ImGui::BeginTable("FlagsTable", 2, ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Active Flags");
                    ImGui::Separator();
                    ImGui::CheckboxFlags("ACTIVE", &activeFlags32, ACTIVE_FLAG_ACTIVE);
                    ImGui::CheckboxFlags("FAR_AWAY", &activeFlags32, ACTIVE_FLAG_FAR_AWAY);
                    ImGui::CheckboxFlags("UNK2", &activeFlags32, ACTIVE_FLAG_UNK2);
                    ImGui::CheckboxFlags("IN_DIFFERENT_ROOM", &activeFlags32, ACTIVE_FLAG_IN_DIFFERENT_ROOM);
                    ImGui::CheckboxFlags("UNIMPORTANT", &activeFlags32, ACTIVE_FLAG_UNIMPORTANT);
                    ImGui::CheckboxFlags("INITIATED_TIME_STOP", &activeFlags32, ACTIVE_FLAG_INITIATED_TIME_STOP);
                    ImGui::CheckboxFlags("MOVE_THROUGH_GRATE", &activeFlags32, ACTIVE_FLAG_MOVE_THROUGH_GRATE);
                    ImGui::CheckboxFlags("DITHERED_ALPHA", &activeFlags32, ACTIVE_FLAG_DITHERED_ALPHA);

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Graphics Flags");
                    ImGui::Separator();
                    ImGui::CheckboxFlags("RENDER_ACTIVE", &gfxFlags32, GRAPH_RENDER_ACTIVE);
                    ImGui::CheckboxFlags("CHILDREN_FIRST", &gfxFlags32, GRAPH_RENDER_CHILDREN_FIRST);
                    ImGui::CheckboxFlags("BILLBOARD", &gfxFlags32, GRAPH_RENDER_BILLBOARD);
                    ImGui::CheckboxFlags("Z_BUFFER", &gfxFlags32, GRAPH_RENDER_Z_BUFFER);
                    ImGui::CheckboxFlags("INVISIBLE", &gfxFlags32, GRAPH_RENDER_INVISIBLE);
                    ImGui::CheckboxFlags("HAS_ANIMATION", &gfxFlags32, GRAPH_RENDER_HAS_ANIMATION);

                    ImGui::EndTable();
                }

                obj->activeFlags = static_cast<s16>(activeFlags32);
                obj->header.gfx.node.flags = static_cast<s16>(gfxFlags32);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Raw Data (0x00-0x4F)")) {
                int tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                                 ImGuiTableFlags_ScrollY;
                if (ImGui::BeginTable("RawDataTable", 6, tableFlags, ImVec2(0.0f, 300.0f))) {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("Idx");
                    ImGui::TableSetupColumn("u32 (Hex)");
                    ImGui::TableSetupColumn("s32 (Int)");
                    ImGui::TableSetupColumn("f32 (Float)");
                    ImGui::TableSetupColumn("s16[0]");
                    ImGui::TableSetupColumn("s16[1]");
                    ImGui::TableHeadersRow();

                    for (int j = 0; j < 0x50; j++) {
                        ImGui::PushID(j);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("[%02X]", j);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputScalar("##u32", ImGuiDataType_U32, &obj->rawData.asU32[j], NULL, NULL, "%08X",
                                           ImGuiInputTextFlags_CharsHexadecimal);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputScalar("##s32", ImGuiDataType_S32, &obj->rawData.asS32[j]);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputScalar("##f32", ImGuiDataType_Float, &obj->rawData.asF32[j]);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputScalar("##s16_0", ImGuiDataType_S16, &obj->rawData.asS16[j][0]);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputScalar("##s16_1", ImGuiDataType_S16, &obj->rawData.asS16[j][1]);
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    };

    for (int i = 0; i < NUM_OBJ_LISTS; i++) {
        const ObjectNode* list = gObjectLists + i;
        if (list->next == list)
            continue;

        bool listHasMatches = false;
        for (ObjectNode* tempNode = list->next; tempNode != list; tempNode = tempNode->next) {
            if (passesFilters(reinterpret_cast<Object*>(tempNode))) {
                listHasMatches = true;
                break;
            }
        }
        if (!listHasMatches)
            continue;

        if (ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(i)), "Object List %d", i)) {
            ObjectNode* node = list->next;
            int objIndex = 0;

            while (node != list) {
                auto obj = reinterpret_cast<Object*>(node);
                node = node->next;

                if (!passesFilters(obj)) {
                    objIndex++;
                    continue;
                }

                ImGui::PushID(obj);

                bool nodeOpen = ImGui::TreeNode(obj, "Object %02d (Model: %d)", objIndex++, obj->modelId);
                // OTRTODO: Implement this later
                // if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                //     obj->header.gfx.node.flags |= GRAPH_RENDER_DRAW_DEBUG;
                // } else {
                //     obj->header.gfx.node.flags &= ~GRAPH_RENDER_DRAW_DEBUG;
                // }

                if (nodeOpen) {
                    drawObjectUI(obj);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
}