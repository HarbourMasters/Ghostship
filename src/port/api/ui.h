#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "api.h"

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

typedef enum { C_ALIGN_LEFT, C_ALIGN_RIGHT } C_ComponentAlign;
typedef enum { C_LABEL_NEAR, C_LABEL_FAR } C_LabelPos;

typedef struct {
    int id;
    const char* value;
} C_ComboboxOption;

typedef struct {
    const char* tooltip;
    int default_index;
    int combo_map_id;
    C_ComponentAlign alignment;
    C_LabelPos label_pos;
    C_ComboboxOption* map;
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
} C_ButtonOpts;

typedef struct {
    C_WidgetType type;
    const char* cvar;
    const char* window_name;

    bool race_disable;
    bool same_line;
    bool hide_in_search;
    bool is_hidden;

    void (*callback)(void);
    void (*pre_func)(void);
    void (*post_func)(void);

    union {
        C_ComboboxOpts combo;
        C_SliderIntOpts slider_int;
        C_SliderFloatOpts slider_float;
        C_CheckboxOpts checkbox;
        C_TextOpts text;
        C_ColorPickerOpts color_picker;
        C_ButtonOpts generic;
    } opts;
} C_WidgetConfig;

GS_API void C_AddSidebarEntry(const char* label, int column);
#define C_AddWidget(sidebar, column, label, config) \
    C_AddWidgetSized((sidebar), (column), (label), (config), sizeof(*(config)))
GS_API void C_AddWidgetSized(const char* sidebar, int column, const char* label, C_WidgetConfig* config, uint32_t size);
GS_API void C_RemoveSidebarEntry(const char* sidebarName);

typedef void (*C_GuiDrawFunc)(void);
GS_API void C_AddGuiDraw(C_GuiDrawFunc callback);
GS_API void C_RemoveGuiDraw(C_GuiDrawFunc callback);

// Drawing with Dear ImGui, from a C_AddGuiDraw callback only. Positions and sizes are in the window's pixels and
// colors are 0xRRGGBBAA. Every C_GuiBegin needs a C_GuiEnd, whatever it returned; a callback that leaves one open
// has it closed for it.

enum {
    C_GUI_NO_TITLE = 1 << 0,
    C_GUI_NO_RESIZE = 1 << 1,
    C_GUI_NO_MOVE = 1 << 2,
    C_GUI_NO_BACKGROUND = 1 << 3,
    C_GUI_AUTO_RESIZE = 1 << 4,
    C_GUI_NO_INPUT = 1 << 5,
    C_GUI_NO_SCROLLBAR = 1 << 6,
    C_GUI_NO_FOCUS = 1 << 7,
};

// A window. open, when not NULL, gets a close button and turns FALSE when it is pressed. Returns FALSE while the
// window is collapsed or hidden: skip its contents, but still call C_GuiEnd.
GS_API int C_GuiBegin(const char* title, int* open, int flags);
GS_API void C_GuiEnd(void);
// For the next C_GuiBegin. always TRUE sets it every frame, FALSE only the first time the window appears.
GS_API void C_GuiSetNextWindowPos(float x, float y, int always);
GS_API void C_GuiSetNextWindowSize(float width, float height, int always);
// A scrolling area inside a window; 0 for a size fills the space left.
GS_API int C_GuiBeginChild(const char* id, float width, float height, int border);
GS_API void C_GuiEndChild(void);

GS_API void C_GuiText(const char* text);
GS_API void C_GuiTextColored(uint32_t color, const char* text);
GS_API void C_GuiTextWrapped(const char* text);
// TRUE on the frame it is clicked.
GS_API int C_GuiButton(const char* label);
// Edits buffer, of size bytes. Returns TRUE when the text changed, or with submit_on_enter when Enter is pressed.
GS_API int C_GuiInputText(const char* label, char* buffer, uint32_t size, int submit_on_enter);
// The following return TRUE when the value changed.
GS_API int C_GuiCheckbox(const char* label, int* value);
GS_API int C_GuiSliderInt(const char* label, int* value, int min, int max);
GS_API int C_GuiSliderFloat(const char* label, float* value, float min, float max);
GS_API void C_GuiSameLine(void);
GS_API void C_GuiSeparator(void);
GS_API void C_GuiSpacing(void);
// Gives the keyboard to the next widget.
GS_API void C_GuiFocusNext(void);
// Scrolls the current window or child to its end, e.g. after adding a chat line.
GS_API void C_GuiScrollToBottom(void);
// Tells apart widgets with the same label, e.g. one per player.
GS_API void C_GuiPushId(const char* id);
GS_API void C_GuiPopId(void);

// Drawn on top of the game and every window.
GS_API void C_GuiDrawText(float x, float y, uint32_t color, const char* text);
GS_API void C_GuiDrawRect(float x0, float y0, float x1, float y1, uint32_t color, float rounding, int filled);
GS_API void C_GuiTextSize(const char* text, float* width, float* height);
GS_API void C_GuiDisplaySize(float* width, float* height);
// TRUE while a text box has the keyboard, so the mod can ignore its own key shortcuts.
GS_API int C_GuiWantsKeyboard(void);

// Where a point in the game world shows on screen, in the same pixels as the drawing above, e.g. for a name over a
// character. FALSE when no level was drawn in the last frame or the point is behind the camera.
GS_API int C_WorldToScreen(float x, float y, float z, float* screen_x, float* screen_y);
