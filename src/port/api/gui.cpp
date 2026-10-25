#ifdef ENABLE_SCRIPTING

#include "ui.h"
#include "port/ShipCompat.h"

#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <imgui.h>

extern "C" int geo_project_to_viewport(float x, float y, float z, float* outX, float* outY);

namespace {

ImU32 ToImColor(uint32_t rgba) {
    return IM_COL32((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}

ImVec4 ToImVec4(uint32_t rgba) {
    return ImGui::ColorConvertU32ToFloat4(ToImColor(rgba));
}

ImGuiWindowFlags ToWindowFlags(int flags) {
    ImGuiWindowFlags out = ImGuiWindowFlags_None;
    if (flags & C_GUI_NO_TITLE) {
        out |= ImGuiWindowFlags_NoTitleBar;
    }
    if (flags & C_GUI_NO_RESIZE) {
        out |= ImGuiWindowFlags_NoResize;
    }
    if (flags & C_GUI_NO_MOVE) {
        out |= ImGuiWindowFlags_NoMove;
    }
    if (flags & C_GUI_NO_BACKGROUND) {
        out |= ImGuiWindowFlags_NoBackground;
    }
    if (flags & C_GUI_AUTO_RESIZE) {
        out |= ImGuiWindowFlags_AlwaysAutoResize;
    }
    if (flags & C_GUI_NO_INPUT) {
        out |= ImGuiWindowFlags_NoInputs;
    }
    if (flags & C_GUI_NO_SCROLLBAR) {
        out |= ImGuiWindowFlags_NoScrollbar;
    }
    if (flags & C_GUI_NO_FOCUS) {
        out |= ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    return out;
}

const char* Str(const char* s) {
    return s != nullptr ? s : "";
}

} // namespace

extern "C" int C_GuiBegin(const char* title, int* open, int flags) {
    if (open == nullptr) {
        return ImGui::Begin(Str(title), nullptr, ToWindowFlags(flags));
    }
    bool shown = *open != 0;
    const bool visible = ImGui::Begin(Str(title), &shown, ToWindowFlags(flags));
    *open = shown;
    return visible;
}

extern "C" void C_GuiEnd(void) {
    ImGui::End();
}

extern "C" void C_GuiSetNextWindowPos(float x, float y, int always) {
    ImGui::SetNextWindowPos(ImVec2(x, y), always ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
}

extern "C" void C_GuiSetNextWindowSize(float width, float height, int always) {
    ImGui::SetNextWindowSize(ImVec2(width, height), always ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
}

extern "C" int C_GuiBeginChild(const char* id, float width, float height, int border) {
    return ImGui::BeginChild(Str(id), ImVec2(width, height), border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None);
}

extern "C" void C_GuiEndChild(void) {
    ImGui::EndChild();
}

extern "C" void C_GuiText(const char* text) {
    ImGui::TextUnformatted(Str(text));
}

extern "C" void C_GuiTextColored(uint32_t color, const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(color));
    ImGui::TextUnformatted(Str(text));
    ImGui::PopStyleColor();
}

extern "C" void C_GuiTextWrapped(const char* text) {
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(Str(text));
    ImGui::PopTextWrapPos();
}

extern "C" int C_GuiButton(const char* label) {
    return ImGui::Button(Str(label));
}

extern "C" int C_GuiInputText(const char* label, char* buffer, uint32_t size, int submit_on_enter) {
    if (buffer == nullptr || size == 0) {
        return 0;
    }
    const ImGuiInputTextFlags flags = submit_on_enter ? ImGuiInputTextFlags_EnterReturnsTrue : ImGuiInputTextFlags_None;
    return ImGui::InputText(Str(label), buffer, size, flags);
}

extern "C" int C_GuiCheckbox(const char* label, int* value) {
    if (value == nullptr) {
        return 0;
    }
    bool checked = *value != 0;
    const bool changed = ImGui::Checkbox(Str(label), &checked);
    *value = checked;
    return changed;
}

extern "C" int C_GuiSliderInt(const char* label, int* value, int min, int max) {
    return value != nullptr && ImGui::SliderInt(Str(label), value, min, max);
}

extern "C" int C_GuiSliderFloat(const char* label, float* value, float min, float max) {
    return value != nullptr && ImGui::SliderFloat(Str(label), value, min, max);
}

extern "C" void C_GuiSameLine(void) {
    ImGui::SameLine();
}

extern "C" void C_GuiSeparator(void) {
    ImGui::Separator();
}

extern "C" void C_GuiSpacing(void) {
    ImGui::Spacing();
}

extern "C" void C_GuiFocusNext(void) {
    ImGui::SetKeyboardFocusHere();
}

extern "C" void C_GuiScrollToBottom(void) {
    ImGui::SetScrollHereY(1.0f);
}

extern "C" void C_GuiPushId(const char* id) {
    ImGui::PushID(Str(id));
}

extern "C" void C_GuiPopId(void) {
    ImGui::PopID();
}

extern "C" void C_GuiDrawText(float x, float y, uint32_t color, const char* text) {
    ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y), ToImColor(color), Str(text));
}

extern "C" void C_GuiDrawRect(float x0, float y0, float x1, float y1, uint32_t color, float rounding, int filled) {
    ImDrawList* list = ImGui::GetForegroundDrawList();
    if (filled) {
        list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), ToImColor(color), rounding);
    } else {
        list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), ToImColor(color), rounding);
    }
}

extern "C" void C_GuiTextSize(const char* text, float* width, float* height) {
    const ImVec2 size = ImGui::CalcTextSize(Str(text));
    if (width != nullptr) {
        *width = size.x;
    }
    if (height != nullptr) {
        *height = size.y;
    }
}

extern "C" void C_GuiDisplaySize(float* width, float* height) {
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    if (width != nullptr) {
        *width = size.x;
    }
    if (height != nullptr) {
        *height = size.y;
    }
}

extern "C" int C_GuiWantsKeyboard(void) {
    return ImGui::GetIO().WantTextInput;
}

extern "C" int C_WorldToScreen(float x, float y, float z, float* screen_x, float* screen_y) {
    auto window = std::dynamic_pointer_cast<Fast::Fast3dWindow>(ShipCompat::GetWindow());
    auto interpreter = window != nullptr ? window->GetInterpreterWeak().lock() : nullptr;
    float ndc_x;
    float ndc_y;
    if (interpreter == nullptr || !geo_project_to_viewport(x, y, z, &ndc_x, &ndc_y)) {
        return 0;
    }

    const auto& viewport = interpreter->mGameWindowViewport;
    if (viewport.width <= 0 || viewport.height <= 0) {
        return 0;
    }
    if (screen_x != nullptr) {
        *screen_x = viewport.x + (ndc_x * 0.5f + 0.5f) * viewport.width;
    }
    if (screen_y != nullptr) {
        *screen_y = viewport.y + (0.5f - ndc_y * 0.5f) * viewport.height;
    }
    return 1;
}

#endif
