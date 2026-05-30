#include "ui.h"
#include <libultraship/libultraship.h>

#include "port/ui/GhostshipGui.hpp"
#include "port/ui/UIWidgets.hpp"
#include "port/ui/MenuTypes.h"
// Include any mod-specific header that holds your maps (e.g. Rando::StaticData)

#include <vector>
#include <algorithm>

static std::vector<C_GuiDrawFunc> gGuiDrawCallbacks;

void C_RunGuiDrawCallbacks() {
    for (auto& cb : gGuiDrawCallbacks) {
        cb();
    }
}

extern "C" void C_AddGuiDraw(C_GuiDrawFunc cb) {
    if (cb) {
        gGuiDrawCallbacks.push_back(cb);
    }
}

extern "C" void C_RemoveGuiDraw(C_GuiDrawFunc cb) {
    gGuiDrawCallbacks.erase(std::remove(gGuiDrawCallbacks.begin(), gGuiDrawCallbacks.end(), cb),
                            gGuiDrawCallbacks.end());
}

// Helper function to map C Enums to C++ Enums safely
static UIWidgets::ComponentAlignments MapAlign(C_ComponentAlign align) {
    if (align == C_ALIGN_RIGHT)
        return UIWidgets::ComponentAlignments::Right;
    return UIWidgets::ComponentAlignments::Left;
}

static UIWidgets::LabelPositions MapLabel(C_LabelPos pos) {
    if (pos == C_LABEL_FAR)
        return UIWidgets::LabelPositions::Far;
    return UIWidgets::LabelPositions::Near;
}

// extern "C" void C_AddMenuEntry(const char* label, const char* cvar) {
//     GhostshipGui::mGhostshipMenu->AddMenuEntry(label, cvar);
// }

extern "C" void C_AddSidebarEntry(const char* label, int column) {
    GhostshipGui::mGhostshipMenu->AddSidebarEntry("Mods", label, column);
}

// extern "C" void C_RemoveMenuEntry(const char* entryName) {
//     GhostshipGui::mGhostshipMenu->RemoveMenuEntry(entryName);
// }

extern "C" void C_RemoveSidebarEntry(const char* sidebarName) {
    GhostshipGui::mGhostshipMenu->RemoveSidebarEntry("Mods", sidebarName);
}

extern "C" void C_AddWidget(const char* sidebar, int column, const char* label, C_WidgetConfig* config) {
    if (!config)
        return;

    WidgetPath path = { "Mods", sidebar, static_cast<SectionColumns>(column) };

    // 1. Begin the Widget Builder Chain
    WidgetInfo& widget = GhostshipGui::mGhostshipMenu->AddWidget(path, label, static_cast<WidgetType>(config->type))
                             .RaceDisable(config->race_disable)
                             .SameLine(config->same_line)
                             .HideInSearch(config->hide_in_search);

    // 2. Apply Base Attributes
    if (config->cvar)
        widget.CVar(config->cvar);
    if (config->window_name)
        widget.WindowName(config->window_name);

    // 3. Apply Callbacks
    if (config->callback) {
        widget.Callback([config](WidgetInfo& info) { config->callback(); });
    }
    if (config->pre_func) {
        widget.PreFunc([config](WidgetInfo& info) { config->pre_func(); });
    }
    if (config->post_func) {
        widget.PostFunc([config](WidgetInfo& info) { config->post_func(); });
    }

    // 4. Unpack the Options Variant based on the type
    switch (config->type) {
        // --- COMBOBOXES ---
        case C_WIDGET_CVAR_COMBOBOX:
        case C_WIDGET_COMBOBOX: {
            auto opt = UIWidgets::ComboboxOptions()
                           .Tooltip(config->opts.combo.tooltip ? config->opts.combo.tooltip : "")
                           .DefaultIndex(config->opts.combo.default_index)
                           .ComponentAlignment(MapAlign(config->opts.combo.alignment))
                           .LabelPosition(MapLabel(config->opts.combo.label_pos));
            if (config->opts.combo.map) {
                // Assuming the map is an array of C_ComboboxOption with a known size (e.g., 10)
                std::unordered_map<int32_t, const char*> comboMap;
                C_ComboboxOption* iter = config->opts.combo.map;
                while (iter->value) { // Assuming null-terminated array
                    comboMap[iter->id] = iter->value;
                    iter++;
                }
                opt.ComboMap(comboMap);
            }
            widget.Options(opt);
            break;
        }
        case C_WIDGET_AUDIO_BACKEND:
        case C_WIDGET_VIDEO_BACKEND: {
            auto opt = UIWidgets::ComboboxOptions()
                           .Tooltip(config->opts.combo.tooltip ? config->opts.combo.tooltip : "")
                           .DefaultIndex(config->opts.combo.default_index)
                           .ComponentAlignment(MapAlign(config->opts.combo.alignment))
                           .LabelPosition(MapLabel(config->opts.combo.label_pos));
            widget.Options(opt);
            break;
        }

        // --- INTEGER SLIDERS ---
        case C_WIDGET_SLIDER_INT:
        case C_WIDGET_CVAR_SLIDER_INT: {
            auto opt = UIWidgets::IntSliderOptions()
                           .Tooltip(config->opts.slider_int.tooltip ? config->opts.slider_int.tooltip : "")
                           .Min(config->opts.slider_int.min)
                           .Max(config->opts.slider_int.max)
                           .DefaultValue(config->opts.slider_int.default_val)
                           .Step(config->opts.slider_int.step);

            if (config->opts.slider_int.format)
                opt.Format(config->opts.slider_int.format);
            widget.Options(opt);
            break;
        }

        // --- FLOAT SLIDERS ---
        case C_WIDGET_SLIDER_FLOAT:
        case C_WIDGET_CVAR_SLIDER_FLOAT: {
            auto opt = UIWidgets::FloatSliderOptions()
                           .Tooltip(config->opts.slider_float.tooltip ? config->opts.slider_float.tooltip : "")
                           .Min(config->opts.slider_float.min)
                           .Max(config->opts.slider_float.max)
                           .DefaultValue(config->opts.slider_float.default_val)
                           .Step(config->opts.slider_float.step);

            if (config->opts.slider_float.format)
                opt.Format(config->opts.slider_float.format);
            widget.Options(opt);
            break;
        }

        // --- CHECKBOXES ---
        case C_WIDGET_CHECKBOX:
        case C_WIDGET_CVAR_CHECKBOX: {
            widget.Options(UIWidgets::CheckboxOptions()
                               .Tooltip(config->opts.checkbox.tooltip ? config->opts.checkbox.tooltip : "")
                               .DefaultValue(config->opts.checkbox.default_val));
            break;
        }

        // --- TEXT ---
        case C_WIDGET_TEXT:
        case C_WIDGET_SEPARATOR_TEXT: {
            widget.Options(UIWidgets::TextOptions()
                           // .Center() or whatever alignment method your UIWidgets uses
            );
            break;
        }

        // --- COLOR PICKERS ---
        case C_WIDGET_COLOR_PICKER:
        case C_WIDGET_CVAR_COLOR_PICKER: {
            // Assuming Color_RGBA8 or similar constructor for default color
            widget.Options(UIWidgets::ColorPickerOptions().Tooltip(
                config->opts.color_picker.tooltip ? config->opts.color_picker.tooltip : "")
                           // .DefaultColor(Color_RGBA8(config->opts.color_picker.default_r, ...))
            );
            break;
        }

        // --- BUTTONS / WINDOWS / SEPARATORS ---
        case C_WIDGET_WINDOW_BUTTON: {
            widget.Options(UIWidgets::WindowButtonOptions().Tooltip(
                config->opts.generic.tooltip ? config->opts.generic.tooltip : ""));
            break;
        }

        case C_WIDGET_BUTTON: {
            widget.Options(
                UIWidgets::ButtonOptions().Tooltip(config->opts.generic.tooltip ? config->opts.generic.tooltip : ""));
            break;
        }

        // --- DEFAULT FALLBACK ---
        default: {
            widget.Options(
                UIWidgets::WidgetOptions().Tooltip(config->opts.generic.tooltip ? config->opts.generic.tooltip : ""));
            break;
        }
    }
}