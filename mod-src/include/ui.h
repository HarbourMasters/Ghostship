#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------
// 1. Enums mirroring the C++ Engine (libultraship)
// ---------------------------------------------------------
typedef enum {
    C_WIDGET_CHECKBOX,
    C_WIDGET_COMBOBOX,
    C_WIDGET_SLIDER_INT,
    C_WIDGET_SLIDER_FLOAT,
    C_WIDGET_CVAR_CHECKBOX,
    C_WIDGET_CVAR_COMBOBOX,
    C_WIDGET_CVAR_SLIDER_INT,
    C_WIDGET_CVAR_SLIDER_FLOAT,
    C_WIDGET_BUTTON,
    C_WIDGET_INPUT,
    C_WIDGET_CVAR_INPUT,
    C_WIDGET_CVAR_COLOR_PICKER,
    C_WIDGET_COLOR_PICKER,
    C_WIDGET_SEARCH,
    C_WIDGET_SEPARATOR,
    C_WIDGET_SEPARATOR_TEXT,
    C_WIDGET_TEXT,
    C_WIDGET_WINDOW_BUTTON,
    C_WIDGET_AUDIO_BACKEND, 
    C_WIDGET_VIDEO_BACKEND, 
    C_WIDGET_CUSTOM,
} C_WidgetType;

typedef enum { C_ALIGN_LEFT, C_ALIGN_RIGHT, C_ALIGN_CENTER } C_ComponentAlign;
typedef enum { C_LABEL_NEAR, C_LABEL_FAR } C_LabelPos;

// ---------------------------------------------------------
// 2. Specific Options Structs
// ---------------------------------------------------------
typedef struct {
    const char* tooltip;
    int default_index;
    int combo_map_id; // Pass an ID so C++ knows which std::map to inject
    C_ComponentAlign alignment;
    C_LabelPos label_pos;
} C_ComboboxOpts;

typedef struct {
    const char* tooltip;
    int min, max, default_val, step;
    const char* format;
} C_SliderIntOpts;

typedef struct {
    const char* tooltip;
    float min, max, default_val, step;
    const char* format;
} C_SliderFloatOpts;

typedef struct {
    const char* tooltip;
    bool default_val;
} C_CheckboxOpts;

typedef struct {
    const char* text;
    bool center;
} C_TextOpts;

typedef struct {
    const char* tooltip;
    uint8_t default_r, default_g, default_b, default_a;
} C_ColorPickerOpts;

typedef struct {
    const char* tooltip;
} C_ButtonOpts; // Also used for generic WidgetOptions

// ---------------------------------------------------------
// 3. The Master Config Struct (Tagged Union)
// ---------------------------------------------------------
typedef struct {
    C_WidgetType type;          // Specifies which union member is active
    const char* cvar;           // Only needed for WIDGET_CVAR_* types
    const char* window_name;    // Used for Window Buttons
    
    bool race_disable;
    bool same_line;
    bool hide_in_search;
    bool is_hidden;
    
    // Callbacks
    void (*callback)(void);     // Fired when widget is interacted with
    void (*pre_func)(void);     // Fired before drawing (for manual disabling/hiding)
    void (*post_func)(void);    // Fired after drawing

    // The Options Union
    union {
        C_ComboboxOpts    combo;
        C_SliderIntOpts   slider_int;
        C_SliderFloatOpts slider_float;
        C_CheckboxOpts    checkbox;
        C_TextOpts        text;
        C_ColorPickerOpts color_picker;
        C_ButtonOpts      generic;
    } opts;
} C_WidgetConfig;

// ---------------------------------------------------------
// 4. The Wrapper Function
// ---------------------------------------------------------
void C_AddWidget(const char* path, const char* label, C_WidgetConfig* config);

#ifdef __cplusplus
}
#endif