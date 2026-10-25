#ifdef ENABLE_SCRIPTING

#include "ui.h"

#include "port/ShipInit.hpp"
#include "port/api/ModOwner.hpp"
#include "port/api/SizedDef.hpp"
#include "port/ui/MenuTypes.h"
#include "port/ui/GhostshipGui.hpp"
#include "port/ui/UIWidgets.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct GuiDraw {
    C_GuiDrawFunc callback;
    std::string owner;
};

struct OwnedSidebar {
    std::string name;
    std::string owner;
};

struct OwnedWidget {
    WidgetPath path;
    std::string label;
    std::string owner;
};

std::vector<GuiDraw> sGuiDrawCallbacks;
std::vector<OwnedSidebar> sSidebars;
std::vector<OwnedWidget> sWidgets;

UIWidgets::ComponentAlignments MapAlign(C_ComponentAlign align) {
    return align == C_ALIGN_RIGHT ? UIWidgets::ComponentAlignments::Right : UIWidgets::ComponentAlignments::Left;
}

UIWidgets::LabelPositions MapLabel(C_LabelPos pos) {
    return pos == C_LABEL_FAR ? UIWidgets::LabelPositions::Far : UIWidgets::LabelPositions::Near;
}

const char* Tooltip(const char* tooltip) {
    return tooltip != nullptr ? tooltip : "";
}

WidgetFunc InOwner(void (*func)(void), const std::string& owner) {
    return [func, owner](WidgetInfo&) {
        ModOwner::Scope scope(owner);
        func();
    };
}

void RemoveSidebar(const std::string& name) {
    if (GhostshipGui::mGhostshipMenu != nullptr) {
        GhostshipGui::mGhostshipMenu->RemoveSidebarEntry("Mods", name);
    }
    std::erase_if(sSidebars, [&](const OwnedSidebar& sidebar) { return sidebar.name == name; });
    std::erase_if(sWidgets, [&](const OwnedWidget& widget) { return widget.path.sidebarName == name; });
}

void ReleaseUi(const std::string& mod) {
    std::erase_if(sGuiDrawCallbacks, [&](const GuiDraw& draw) { return draw.owner == mod; });
    for (const OwnedWidget& widget : sWidgets) {
        if (widget.owner == mod && GhostshipGui::mGhostshipMenu != nullptr) {
            GhostshipGui::mGhostshipMenu->RemoveWidget(widget.path, widget.label);
        }
    }
    std::erase_if(sWidgets, [&](const OwnedWidget& widget) { return widget.owner == mod; });
    std::vector<std::string> sidebars;
    for (const OwnedSidebar& sidebar : sSidebars) {
        if (sidebar.owner == mod) {
            sidebars.push_back(sidebar.name);
        }
    }
    for (const std::string& name : sidebars) {
        RemoveSidebar(name);
    }
}

void InitUiApi() {
    ModOwner::AddReleaser(ReleaseUi);
}

RegisterShipInitFunc sInitUiApi(InitUiApi);

} // namespace

void C_RunGuiDrawCallbacks() {
    for (const GuiDraw& draw : sGuiDrawCallbacks) {
        ModOwner::Scope scope(draw.owner);
        ImGuiErrorRecoveryState state;
        ImGui::ErrorRecoveryStoreState(&state);
        draw.callback();
        ImGui::ErrorRecoveryTryToRecoverState(&state);
    }
}

extern "C" void C_AddGuiDraw(C_GuiDrawFunc callback) {
    if (callback != nullptr) {
        sGuiDrawCallbacks.push_back({ callback, ModOwner::Current() });
    }
}

extern "C" void C_RemoveGuiDraw(C_GuiDrawFunc callback) {
    std::erase_if(sGuiDrawCallbacks, [callback](const GuiDraw& draw) { return draw.callback == callback; });
}

extern "C" void C_AddSidebarEntry(const char* label, int column) {
    if (GhostshipGui::mGhostshipMenu == nullptr || label == nullptr) {
        return;
    }
    GhostshipGui::mGhostshipMenu->AddSidebarEntry("Mods", label, column);
    sSidebars.push_back({ label, ModOwner::Current() });
}

extern "C" void C_RemoveSidebarEntry(const char* sidebarName) {
    if (GhostshipGui::mGhostshipMenu == nullptr || sidebarName == nullptr) {
        return;
    }
    RemoveSidebar(sidebarName);
}

extern "C" void C_AddWidgetSized(const char* sidebar, int column, const char* label, C_WidgetConfig* widget_config,
                                 uint32_t size) {
    const SizedDef<C_WidgetConfig> sized(widget_config, size);
    const C_WidgetConfig* config = sized.get();
    if (GhostshipGui::mGhostshipMenu == nullptr || sidebar == nullptr || label == nullptr || config == nullptr) {
        return;
    }

    const std::string owner = ModOwner::Current();
    WidgetPath path = { "Mods", sidebar, static_cast<SectionColumns>(column) };
    sWidgets.push_back({ path, label, owner });
    WidgetInfo& widget = GhostshipGui::mGhostshipMenu->AddWidget(path, label, static_cast<WidgetType>(config->type))
                             .RaceDisable(config->race_disable)
                             .SameLine(config->same_line)
                             .HideInSearch(config->hide_in_search);

    if (config->cvar != nullptr) {
        widget.CVar(config->cvar);
    }
    if (config->window_name != nullptr) {
        widget.WindowName(config->window_name);
    }
    if (config->callback != nullptr) {
        widget.Callback(InOwner(config->callback, owner));
    }
    if (config->pre_func != nullptr) {
        widget.PreFunc(InOwner(config->pre_func, owner));
    }
    if (config->post_func != nullptr) {
        widget.PostFunc(InOwner(config->post_func, owner));
    }

    switch (config->type) {
        case C_WIDGET_CVAR_COMBOBOX:
        case C_WIDGET_COMBOBOX: {
            auto options = UIWidgets::ComboboxOptions()
                               .Tooltip(Tooltip(config->opts.combo.tooltip))
                               .DefaultIndex(config->opts.combo.default_index)
                               .ComponentAlignment(MapAlign(config->opts.combo.alignment))
                               .LabelPosition(MapLabel(config->opts.combo.label_pos));
            if (config->opts.combo.map != nullptr) {
                std::unordered_map<int32_t, const char*> comboMap;
                for (C_ComboboxOption* it = config->opts.combo.map; it->value != nullptr; it++) {
                    comboMap[it->id] = it->value;
                }
                options.ComboMap(comboMap);
            }
            widget.Options(options);
            break;
        }
        case C_WIDGET_AUDIO_BACKEND:
        case C_WIDGET_VIDEO_BACKEND:
            widget.Options(UIWidgets::ComboboxOptions()
                               .Tooltip(Tooltip(config->opts.combo.tooltip))
                               .DefaultIndex(config->opts.combo.default_index)
                               .ComponentAlignment(MapAlign(config->opts.combo.alignment))
                               .LabelPosition(MapLabel(config->opts.combo.label_pos)));
            break;
        case C_WIDGET_SLIDER_INT:
        case C_WIDGET_CVAR_SLIDER_INT: {
            auto options = UIWidgets::IntSliderOptions()
                               .Tooltip(Tooltip(config->opts.slider_int.tooltip))
                               .Min(config->opts.slider_int.min)
                               .Max(config->opts.slider_int.max)
                               .DefaultValue(config->opts.slider_int.default_val)
                               .Step(config->opts.slider_int.step);
            if (config->opts.slider_int.format != nullptr) {
                options.Format(config->opts.slider_int.format);
            }
            widget.Options(options);
            break;
        }
        case C_WIDGET_SLIDER_FLOAT:
        case C_WIDGET_CVAR_SLIDER_FLOAT: {
            auto options = UIWidgets::FloatSliderOptions()
                               .Tooltip(Tooltip(config->opts.slider_float.tooltip))
                               .Min(config->opts.slider_float.min)
                               .Max(config->opts.slider_float.max)
                               .DefaultValue(config->opts.slider_float.default_val)
                               .Step(config->opts.slider_float.step);
            if (config->opts.slider_float.format != nullptr) {
                options.Format(config->opts.slider_float.format);
            }
            widget.Options(options);
            break;
        }
        case C_WIDGET_CHECKBOX:
        case C_WIDGET_CVAR_CHECKBOX:
            widget.Options(UIWidgets::CheckboxOptions()
                               .Tooltip(Tooltip(config->opts.checkbox.tooltip))
                               .DefaultValue(config->opts.checkbox.default_val));
            break;
        case C_WIDGET_TEXT:
        case C_WIDGET_SEPARATOR_TEXT:
            widget.Options(UIWidgets::TextOptions());
            break;
        case C_WIDGET_COLOR_PICKER:
        case C_WIDGET_CVAR_COLOR_PICKER:
            widget.Options(UIWidgets::ColorPickerOptions().Tooltip(Tooltip(config->opts.color_picker.tooltip)));
            break;
        case C_WIDGET_WINDOW_BUTTON:
            widget.Options(UIWidgets::WindowButtonOptions().Tooltip(Tooltip(config->opts.generic.tooltip)));
            break;
        case C_WIDGET_BUTTON:
            widget.Options(UIWidgets::ButtonOptions().Tooltip(Tooltip(config->opts.generic.tooltip)));
            break;
        default:
            widget.Options(UIWidgets::WidgetOptions().Tooltip(Tooltip(config->opts.generic.tooltip)));
            break;
    }
}

#endif
