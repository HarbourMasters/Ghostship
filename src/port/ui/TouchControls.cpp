#include "TouchControls.h"
#include "port/ShipCompat.h"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/libultra/controller.h>
#include <ship/core/Context.h>
#include <fast/Fast3dGui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#if defined(PLATFORM_IOS) || defined(__IOS__) || defined(__ANDROID__)
#define TOUCH_CONTROLS_DEFAULT 1
#else
#define TOUCH_CONTROLS_DEFAULT 0
#endif

namespace {

constexpr float kStickMax = 80.0f; // N64 raw stick range used by the game

// The button PNGs (shared with InputViewer) are 327x175 full-frame layers with
// one element each; individual buttons are UV-cropped out via the element
// bounding boxes measured from the source art.
constexpr float kFrameW = 327.0f;
constexpr float kFrameH = 175.0f;

struct Rect {
    float x0, y0, x1, y1;
};

// dirX/dirY drive the vector fallback arrow when a C-button texture is missing.
// id is the stable key custom positions are persisted under.
struct TouchButton {
    uint16_t mask;
    const char* id;
    const char* label;
    const char* texName;
    const char* texPath;
    const char* outlineName;
    const char* outlinePath;
    ImVec2 uv0;
    ImVec2 uv1;
    ImU32 color;
    float dirX;
    float dirY;
    ImVec2 center;
    float halfW;
    float halfH;
    bool pressed;
};

struct Finger {
    ImVec2 pos;
    int64_t id;
};

struct OverlayState {
    bool active = false;
    bool menuOpen = false;
    uint16_t buttons = 0;
    int8_t stickX = 0;
    int8_t stickY = 0;
    bool stickHeld = false;
    ImVec2 stickAnchor;
    ImVec2 stickPos;
    ImVec2 stickRest;
    float stickTravel = 0.0f;
    float nubRadius = 0.0f;
    ImVec2 stickUv0;
    ImVec2 stickUv1;
    ImVec2 menuCenter;
    float menuRadius = 0.0f;
    bool menuDown = false;
    // Layout-edit mode: drag widgets to reposition; Done saves, Reset restores.
    bool editMode = false;
    ImVec2 doneCenter, doneHalf;
    ImVec2 resetCenter, resetHalf;
    std::vector<TouchButton> gameButtons;
};

OverlayState sState;
int64_t sStickFinger = -1;
constexpr int64_t kMouseFingerId = -0x4D6F7573; // synthetic finger for desktop testing

// Edit-mode drag state. Targets: 0..N-1 = gameButtons index, -2 = stick, -3 = menu.
int64_t sDragFinger = -1;
int sDragTarget = -1;
ImVec2 sDragOffset;
ImVec2 sDragPos; // last dragged position; ComputeLayout resets widget centers each tick
bool sDoneDown = false;
bool sResetDown = false;

void LayoutKey(char* out, size_t outSize, const char* id, const char* axis) {
    snprintf(out, outSize, CVAR_TOUCH("Layout.%s.%s"), id, axis);
}

// Custom positions are stored normalized (0..1 of display) so they survive
// resolution and orientation changes.
ImVec2 LoadPos(const char* id, ImVec2 fallback, float w, float h) {
    char kx[64], ky[64];
    LayoutKey(kx, sizeof(kx), id, "X");
    LayoutKey(ky, sizeof(ky), id, "Y");
    const float nx = CVarGetFloat(kx, -1.0f);
    const float ny = CVarGetFloat(ky, -1.0f);
    if (nx >= 0.0f && ny >= 0.0f) {
        return ImVec2(nx * w, ny * h);
    }
    return fallback;
}

void SavePos(const char* id, ImVec2 center, float w, float h) {
    char kx[64], ky[64];
    LayoutKey(kx, sizeof(kx), id, "X");
    LayoutKey(ky, sizeof(ky), id, "Y");
    CVarSetFloat(kx, std::clamp(center.x / w, 0.0f, 1.0f));
    CVarSetFloat(ky, std::clamp(center.y / h, 0.0f, 1.0f));
}

void ClearPos(const char* id) {
    char kx[64], ky[64];
    LayoutKey(kx, sizeof(kx), id, "X");
    LayoutKey(ky, sizeof(ky), id, "Y");
    CVarClear(kx);
    CVarClear(ky);
}

ImVec2 UvMin(const Rect& r) {
    return ImVec2(r.x0 / kFrameW, r.y0 / kFrameH);
}
ImVec2 UvMax(const Rect& r) {
    return ImVec2(r.x1 / kFrameW, r.y1 / kFrameH);
}
float Aspect(const Rect& r) {
    return (r.x1 - r.x0) / (r.y1 - r.y0);
}

void RegisterCVars() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    CVarRegisterInteger(CVAR_TOUCH("Enabled"), TOUCH_CONTROLS_DEFAULT);
    CVarRegisterFloat(CVAR_TOUCH("Scale"), 1.0f);
    CVarRegisterFloat(CVAR_TOUCH("Opacity"), 0.8f);
    // Edit mode never persists across runs.
    CVarSetInteger(CVAR_TOUCH("EditMode"), 0);
}

bool Enabled() {
    RegisterCVars();
    return CVarGetInteger(CVAR_TOUCH("Enabled"), TOUCH_CONTROLS_DEFAULT) != 0;
}

// Widget heights are multiples of the layout unit; widths follow the cropped
// sprite's aspect ratio so the art is never stretched.
void ComputeLayout(OverlayState& state) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float w = display.x;
    const float h = display.y;
    const float scale = std::clamp(CVarGetFloat(CVAR_TOUCH("Scale"), 1.0f), 0.5f, 2.0f);
    const float u = std::min(w, h) * 0.055f * scale;

    // Element bounding boxes within the 327x175 frame.
    const Rect rA = { 164, 116, 210, 162 };
    const Rect rB = { 129, 66, 175, 112 };
    const Rect rCUp = { 220, 42, 254, 76 };
    const Rect rCDown = { 220, 102, 254, 136 };
    const Rect rCLeft = { 190, 72, 224, 106 };
    const Rect rCRight = { 250, 72, 284, 106 };
    const Rect rL = { 27, 17, 107, 29 };
    const Rect rR = { 198, 17, 277, 29 };
    const Rect rZ = { 114, 37, 193, 49 };
    const Rect rStart = { 144, 14, 162, 32 };
    const Rect rStick = { 34, 56, 100, 122 };
    // The D-pad textures are one arm each, all sharing the center hub square.
    const Rect rDUp = { 121, 118, 133, 145 };
    const Rect rDDown = { 121, 133, 133, 160 };
    const Rect rDLeft = { 106, 133, 133, 145 };
    const Rect rDRight = { 121, 133, 148, 145 };

    const ImU32 kBlue = IM_COL32(60, 100, 230, 255);
    const ImU32 kGreen = IM_COL32(40, 170, 80, 255);
    const ImU32 kYellow = IM_COL32(250, 190, 30, 255);
    const ImU32 kRed = IM_COL32(220, 60, 60, 255);
    const ImU32 kGray = IM_COL32(150, 150, 160, 255);

    // The default center can be overridden by a saved custom position (edit mode).
    const auto make = [w, h](uint16_t mask, const char* id, const char* label, const char* base, const char* basePath,
                             const char* outline, const char* outlinePath, const Rect& r, ImU32 color, float dirX,
                             float dirY, ImVec2 center, float halfH) {
        TouchButton b{};
        b.mask = mask;
        b.id = id;
        b.label = label;
        b.texName = base;
        b.texPath = basePath;
        b.outlineName = outline;
        b.outlinePath = outlinePath;
        b.uv0 = UvMin(r);
        b.uv1 = UvMax(r);
        b.color = color;
        b.dirX = dirX;
        b.dirY = dirY;
        b.center = LoadPos(id, center, w, h);
        b.halfH = halfH;
        b.halfW = halfH * Aspect(r);
        b.pressed = false;
        return b;
    };

    state.stickTravel = 2.4f * u;
    state.nubRadius = 1.6f * u;
    state.stickRest = LoadPos("Stick", ImVec2(4.8f * u, h - 4.8f * u), w, h);
    state.stickUv0 = UvMin(rStick);
    state.stickUv1 = UvMax(rStick);

    state.menuCenter = LoadPos("Menu", ImVec2(2.0f * u, 1.8f * u), w, h);
    state.menuRadius = 1.0f * u;

    state.editMode = CVarGetInteger(CVAR_TOUCH("EditMode"), 0) != 0;
    state.doneCenter = ImVec2(w * 0.5f + 3.4f * u, 1.6f * u);
    state.doneHalf = ImVec2(2.6f * u, 1.0f * u);
    state.resetCenter = ImVec2(w * 0.5f - 3.4f * u, 1.6f * u);
    state.resetHalf = ImVec2(2.6f * u, 1.0f * u);

    state.gameButtons.clear();
    // Face buttons, bottom-right.
    state.gameButtons.push_back(make(BTN_A, "A", "A", "A-Btn", "textures/buttons/ABtn.png", "A-Btn Outline",
                                     "textures/buttons/ABtnOutline.png", rA, kBlue, 0, 0,
                                     ImVec2(w - 2.6f * u, h - 3.0f * u), 1.5f * u));
    state.gameButtons.push_back(make(BTN_B, "B", "B", "B-Btn", "textures/buttons/BBtn.png", "B-Btn Outline",
                                     "textures/buttons/BBtnOutline.png", rB, kGreen, 0, 0,
                                     ImVec2(w - 5.6f * u, h - 4.2f * u), 1.25f * u));
    // C buttons in a diamond, above the face buttons.
    const ImVec2 c(w - 3.3f * u, h - 8.8f * u);
    const float cOff = 1.5f * u;
    const float cR = 0.85f * u;
    state.gameButtons.push_back(make(BTN_CUP, "CUp", "C", "C-Up", "textures/buttons/CUp.png", "C-Up Outline",
                                     "textures/buttons/CUpOutline.png", rCUp, kYellow, 0, -1, ImVec2(c.x, c.y - cOff),
                                     cR));
    state.gameButtons.push_back(make(BTN_CDOWN, "CDown", "C", "C-Down", "textures/buttons/CDown.png", "C-Down Outline",
                                     "textures/buttons/CDownOutline.png", rCDown, kYellow, 0, 1,
                                     ImVec2(c.x, c.y + cOff), cR));
    state.gameButtons.push_back(make(BTN_CLEFT, "CLeft", "C", "C-Left", "textures/buttons/CLeft.png", "C-Left Outline",
                                     "textures/buttons/CLeftOutline.png", rCLeft, kYellow, -1, 0,
                                     ImVec2(c.x - cOff, c.y), cR));
    state.gameButtons.push_back(make(BTN_CRIGHT, "CRight", "C", "C-Right", "textures/buttons/CRight.png",
                                     "C-Right Outline", "textures/buttons/CRightOutline.png", rCRight, kYellow, 1, 0,
                                     ImVec2(c.x + cOff, c.y), cR));
    // Shoulder / trigger bars: L top-left (clear of the menu button), R and Z top-right.
    state.gameButtons.push_back(make(BTN_L, "L", "L", "L-Btn", "textures/buttons/LBtn.png", "L-Btn Outline",
                                     "textures/buttons/LBtnOutline.png", rL, kGray, 0, 0, ImVec2(7.5f * u, 1.6f * u),
                                     0.55f * u));
    state.gameButtons.push_back(make(BTN_R, "R", "R", "R-Btn", "textures/buttons/RBtn.png", "R-Btn Outline",
                                     "textures/buttons/RBtnOutline.png", rR, kGray, 0, 0,
                                     ImVec2(w - 4.2f * u, 1.6f * u), 0.55f * u));
    state.gameButtons.push_back(make(BTN_Z, "Z", "Z", "Z-Btn", "textures/buttons/ZBtn.png", "Z-Btn Outline",
                                     "textures/buttons/ZBtnOutline.png", rZ, kGray, 0, 0,
                                     ImVec2(w - 4.2f * u, 3.4f * u), 0.55f * u));
    // D-pad cross on the left, above the stick area. Each texture is one arm
    // (with the shared hub), so adjacent placement reassembles the cross.
    const ImVec2 d(3.4f * u, h - 9.8f * u);
    const float dOff = 0.75f * u;
    state.gameButtons.push_back(make(BTN_DUP, "DUp", "D", "Dpad-Up", "textures/buttons/DPadUp.png", "Dpad-Up Outline",
                                     "textures/buttons/DPadUpOutline.png", rDUp, kGray, 0, -1, ImVec2(d.x, d.y - dOff),
                                     1.0f * u));
    state.gameButtons.push_back(make(BTN_DDOWN, "DDown", "D", "Dpad-Down", "textures/buttons/DPadDown.png",
                                     "Dpad-Down Outline", "textures/buttons/DPadDownOutline.png", rDDown, kGray, 0, 1,
                                     ImVec2(d.x, d.y + dOff), 1.0f * u));
    state.gameButtons.push_back(make(BTN_DLEFT, "DLeft", "D", "Dpad-Left", "textures/buttons/DPadLeft.png",
                                     "Dpad-Left Outline", "textures/buttons/DPadLeftOutline.png", rDLeft, kGray, -1, 0,
                                     ImVec2(d.x - dOff, d.y), 0.45f * u));
    state.gameButtons.push_back(make(BTN_DRIGHT, "DRight", "D", "Dpad-Right", "textures/buttons/DPadRight.png",
                                     "Dpad-Right Outline", "textures/buttons/DPadRightOutline.png", rDRight, kGray, 1,
                                     0, ImVec2(d.x + dOff, d.y), 0.45f * u));
    // Start, bottom-center.
    state.gameButtons.push_back(make(BTN_START, "Start", "S", "Start-Btn", "textures/buttons/StartBtn.png",
                                     "Start-Btn Outline", "textures/buttons/StartBtnOutline.png", rStart, kRed, 0, 0,
                                     ImVec2(w * 0.5f, h - 1.7f * u), 0.9f * u));
}

float Dist(const ImVec2& a, const ImVec2& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Returns [0,1] inside the padded hit rect (0 = dead center), negative on miss.
// Fingers press only their best-scoring button so tight clusters (D-pad arms)
// never fire opposite directions from one touch.
float HitScore(const TouchButton& b, const ImVec2& p) {
    const float mx = std::max(b.halfW * 1.15f, b.halfH * 1.4f);
    const float my = b.halfH * 1.5f;
    const float nx = std::fabs(p.x - b.center.x) / mx;
    const float ny = std::fabs(p.y - b.center.y) / my;
    const float score = std::max(nx, ny);
    return score <= 1.0f ? score : -1.0f;
}

bool HitsButton(const TouchButton& b, const ImVec2& p) {
    return HitScore(b, p) >= 0.0f;
}

bool InRect(const ImVec2& p, const ImVec2& center, const ImVec2& half) {
    return std::fabs(p.x - center.x) <= half.x && std::fabs(p.y - center.y) <= half.y;
}

// Layout-edit mode: one finger drags a widget; positions commit on release.
// Done exits the mode (saving the config), Reset restores the default layout.
void HandleEditMode(OverlayState& state, const std::vector<Finger>& fingers, float w, float h) {
    // Done / Reset fire on release, like the menu toggle.
    bool doneNow = false;
    bool resetNow = false;
    for (const auto& finger : fingers) {
        if (finger.id != sDragFinger) {
            doneNow |= InRect(finger.pos, state.doneCenter, state.doneHalf);
            resetNow |= InRect(finger.pos, state.resetCenter, state.resetHalf);
        }
    }
    if (sDoneDown && !doneNow) {
        CVarSetInteger(CVAR_TOUCH("EditMode"), 0);
        CVarSave();
        sDragFinger = -1;
        sDragTarget = -1;
        sDoneDown = sResetDown = false;
        return;
    }
    if (sResetDown && !resetNow) {
        for (const auto& button : state.gameButtons) {
            ClearPos(button.id);
        }
        ClearPos("Stick");
        ClearPos("Menu");
        CVarSave();
        sDragFinger = -1;
        sDragTarget = -1;
        sDoneDown = sResetDown = false;
        return;
    }
    sDoneDown = doneNow;
    sResetDown = resetNow;

    // Continue an active drag, or commit it when the finger lifted.
    if (sDragFinger != -1) {
        const Finger* held = nullptr;
        for (const auto& finger : fingers) {
            if (finger.id == sDragFinger) {
                held = &finger;
                break;
            }
        }
        if (held == nullptr) {
            // Released: persist the last dragged position (the state's centers
            // were already reset by ComputeLayout this tick).
            if (sDragTarget >= 0 && sDragTarget < (int)state.gameButtons.size()) {
                SavePos(state.gameButtons[sDragTarget].id, sDragPos, w, h);
            } else if (sDragTarget == -2) {
                SavePos("Stick", sDragPos, w, h);
            } else if (sDragTarget == -3) {
                SavePos("Menu", sDragPos, w, h);
            }
            CVarSave();
            sDragFinger = -1;
            sDragTarget = -1;
            return;
        }
        sDragPos =
            ImVec2(std::clamp(held->pos.x + sDragOffset.x, 0.0f, w), std::clamp(held->pos.y + sDragOffset.y, 0.0f, h));
        if (sDragTarget >= 0 && sDragTarget < (int)state.gameButtons.size()) {
            state.gameButtons[sDragTarget].center = sDragPos;
        } else if (sDragTarget == -2) {
            state.stickRest = sDragPos;
        } else if (sDragTarget == -3) {
            state.menuCenter = sDragPos;
        }
        return;
    }

    // Begin a drag on the first finger that lands on a widget.
    for (const auto& finger : fingers) {
        if (InRect(finger.pos, state.doneCenter, state.doneHalf) ||
            InRect(finger.pos, state.resetCenter, state.resetHalf)) {
            continue;
        }
        for (int i = 0; i < (int)state.gameButtons.size(); i++) {
            if (HitsButton(state.gameButtons[i], finger.pos)) {
                sDragFinger = finger.id;
                sDragTarget = i;
                sDragOffset =
                    ImVec2(state.gameButtons[i].center.x - finger.pos.x, state.gameButtons[i].center.y - finger.pos.y);
                sDragPos = state.gameButtons[i].center;
                return;
            }
        }
        if (Dist(finger.pos, state.stickRest) <= state.stickTravel) {
            sDragFinger = finger.id;
            sDragTarget = -2;
            sDragOffset = ImVec2(state.stickRest.x - finger.pos.x, state.stickRest.y - finger.pos.y);
            sDragPos = state.stickRest;
            return;
        }
        if (Dist(finger.pos, state.menuCenter) <= state.menuRadius * 1.3f) {
            sDragFinger = finger.id;
            sDragTarget = -3;
            sDragOffset = ImVec2(state.menuCenter.x - finger.pos.x, state.menuCenter.y - finger.pos.y);
            sDragPos = state.menuCenter;
            return;
        }
    }
}

std::vector<Finger> GatherFingers() {
    std::vector<Finger> fingers;
    const ImVec2 display = ImGui::GetIO().DisplaySize;

    int deviceCount = 0;
    SDL_TouchID* devices = SDL_GetTouchDevices(&deviceCount);
    if (devices != nullptr) {
        for (int d = 0; d < deviceCount; d++) {
            const SDL_TouchID device = devices[d];
            if (device == 0) {
                continue;
            }
            int fingerCount = 0;
            SDL_Finger** touchFingers = SDL_GetTouchFingers(device, &fingerCount);
            if (touchFingers != nullptr) {
                for (int f = 0; f < fingerCount; f++) {
                    const SDL_Finger* finger = touchFingers[f];
                    if (finger != nullptr) {
                        fingers.push_back({ ImVec2(finger->x * display.x, finger->y * display.y), (int64_t)finger->id });
                    }
                }
                SDL_free(touchFingers);
            }
        }
        SDL_free(devices);
    }

#if !defined(PLATFORM_IOS) && !defined(__IOS__) && !defined(__ANDROID__)
    // Desktop testing: the mouse acts as a single finger while held down.
    const ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDown[ImGuiMouseButton_Left]) {
        fingers.push_back({ io.MousePos, kMouseFingerId });
    }
#endif

    return fingers;
}

} // namespace

extern "C" void TouchControls_ApplyPad(void* pads) {
    sState.active = false;
    sState.buttons = 0;
    sState.stickHeld = false;
    sState.stickX = 0;
    sState.stickY = 0;
    for (auto& button : sState.gameButtons) {
        button.pressed = false;
    }

    if (ImGui::GetCurrentContext() == nullptr || !Enabled()) {
        sStickFinger = -1;
        return;
    }

    const auto window = ShipCompat::GetWindow();
    if (window == nullptr || window->GetGui() == nullptr) {
        sStickFinger = -1;
        return;
    }

    const auto gui = window->GetGui();
    const auto menu = gui->GetMenu();
    sState.menuOpen = menu != nullptr && menu->IsVisible();
    sState.active = true;

    ComputeLayout(sState);
    const std::vector<Finger> fingers = GatherFingers();

    // Layout-edit mode: fingers drag widgets instead of driving the game.
    if (sState.editMode && !sState.menuOpen) {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        HandleEditMode(sState, fingers, display.x, display.y);
        return;
    }

    // Menu toggle fires on release and works in every state.
    bool menuDownNow = false;
    for (const auto& finger : fingers) {
        if (Dist(finger.pos, sState.menuCenter) <= sState.menuRadius * 1.3f) {
            menuDownNow = true;
            break;
        }
    }
    if (sState.menuDown && !menuDownNow && menu != nullptr) {
        menu->ToggleVisibility();
        sState.menuOpen = !sState.menuOpen;
    }
    sState.menuDown = menuDownNow;

    if (sState.menuOpen) {
        sStickFinger = -1;
        return;
    }

    // Each finger presses its closest hit; re-evaluated every tick so sliding works.
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    std::vector<const Finger*> freeFingers;
    for (const auto& finger : fingers) {
        bool consumed = false;
        int best = -1;
        float bestScore = 2.0f;
        for (int i = 0; i < (int)sState.gameButtons.size(); i++) {
            const float score = HitScore(sState.gameButtons[i], finger.pos);
            if (score >= 0.0f && score < bestScore) {
                bestScore = score;
                best = i;
            }
        }
        if (best >= 0) {
            sState.gameButtons[best].pressed = true;
            sState.buttons |= sState.gameButtons[best].mask;
            consumed = true;
        }
        if (Dist(finger.pos, sState.menuCenter) <= sState.menuRadius * 1.3f) {
            consumed = true;
        }
        if (!consumed) {
            freeFingers.push_back(&finger);
        }
    }

    // Floating stick: a free finger in the lower-left region anchors it there.
    const Finger* stickNow = nullptr;
    for (const Finger* finger : freeFingers) {
        if (finger->id == sStickFinger) {
            stickNow = finger;
            break;
        }
    }
    if (stickNow == nullptr) {
        sStickFinger = -1;
        for (const Finger* finger : freeFingers) {
            if (finger->pos.x < display.x * 0.45f && finger->pos.y > display.y * 0.25f) {
                sStickFinger = finger->id;
                sState.stickAnchor = finger->pos;
                stickNow = finger;
                break;
            }
        }
    }

    if (stickNow != nullptr) {
        float dx = (stickNow->pos.x - sState.stickAnchor.x) / sState.stickTravel;
        float dy = (stickNow->pos.y - sState.stickAnchor.y) / sState.stickTravel;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1.0f) {
            dx /= len;
            dy /= len;
        }
        sState.stickHeld = true;
        sState.stickX = (int8_t)std::clamp(dx * kStickMax, -kStickMax, kStickMax);
        sState.stickY = (int8_t)std::clamp(-dy * kStickMax, -kStickMax, kStickMax); // screen y is down, stick y is up
        sState.stickPos = ImVec2(sState.stickAnchor.x + dx * sState.stickTravel * std::min(len, 1.0f),
                                 sState.stickAnchor.y + dy * sState.stickTravel * std::min(len, 1.0f));
    }

    if (pads == nullptr || ShipCompat::GetControlDeck() == nullptr ||
        ShipCompat::GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    OSContPad* pad = static_cast<OSContPad*>(pads);
    pad->button |= sState.buttons;
    if (sState.stickHeld && pad->stick_x == 0 && pad->stick_y == 0) {
        pad->stick_x = sState.stickX;
        pad->stick_y = sState.stickY;
    }
}

namespace GhostshipGui {

void TouchControlsOverlay::OnInit(const nlohmann::json& initArgs) {
    Ship::GuiWindow::OnInit(initArgs);
    RegisterCVars();
}

void TouchControlsOverlay::Draw() {
    if (!sState.active || !Enabled()) {
        return;
    }

    const float opacity = std::clamp(CVarGetFloat(CVAR_TOUCH("Opacity"), 0.8f), 0.1f, 1.0f);
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const auto gui = std::static_pointer_cast<Fast::Fast3dGui>(ShipCompat::GetWindow()->GetGui());

    // Registry names shared with InputViewer; retry while archives settle.
    static int textureAttempts = 120;
    if (textureAttempts > 0) {
        bool missing = false;
        const auto ensure = [&](const char* name, const char* path) {
            if (gui->GetTextureByName(name) == nullptr) {
                missing = true;
                gui->LoadTextureFromRawImage(name, path);
            }
        };
        for (const auto& button : sState.gameButtons) {
            ensure(button.texName, button.texPath);
            ensure(button.outlineName, button.outlinePath);
        }
        ensure("Analog-Stick", "textures/buttons/AnalogStick.png");
        ensure("Analog-Stick Outline", "textures/buttons/AnalogStickOutline.png");
        textureAttempts = missing ? textureAttempts - 1 : 0;
        if (textureAttempts == 1) {
            SPDLOG_WARN("TouchControls: button textures failed to load; using vector fallback");
        }
    }

    const float u = std::min(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y) * 0.055f *
                    std::clamp(CVarGetFloat(CVAR_TOUCH("Scale"), 1.0f), 0.5f, 2.0f);
    const float stroke = std::max(2.0f, u * 0.09f);
    ImFont* font = ImGui::GetFont();

    const auto alpha = [opacity](float a) { return (ImU32)(std::min(a, 1.0f) * opacity * 255.0f) << IM_COL32_A_SHIFT; };
    const auto rgb = [](ImU32 c) { return c & ~IM_COL32_A_MASK; };
    const auto text = [&](const ImVec2& center, float size, const char* s, ImU32 col) {
        const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
        drawList->AddText(font, size, ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f), col, s);
    };
    // Draws a cropped sprite region centered on a button.
    const auto sprite = [&](ImTextureID tex, const TouchButton& b, const ImVec2& uv0, const ImVec2& uv1, float a) {
        const ImVec2 min(b.center.x - b.halfW, b.center.y - b.halfH);
        const ImVec2 max(b.center.x + b.halfW, b.center.y + b.halfH);
        drawList->AddImage(tex, min, max, uv0, uv1, IM_COL32(255, 255, 255, 0) | alpha(a));
    };

    // Menu toggle, drawn in every state (including while the menu is open).
    drawList->AddCircleFilled(sState.menuCenter, sState.menuRadius, IM_COL32(20, 20, 26, 0) | alpha(0.55f));
    drawList->AddCircle(sState.menuCenter, sState.menuRadius, IM_COL32(255, 255, 255, 0) | alpha(0.85f), 0, stroke);
    text(sState.menuCenter, sState.menuRadius * 1.1f, "=", IM_COL32(255, 255, 255, 0) | alpha(0.9f));

    if (sState.menuOpen) {
        return;
    }

    for (const auto& button : sState.gameButtons) {
        ImTextureID solidTex = gui->GetTextureByName(button.texName);
        ImTextureID outlineTex = gui->GetTextureByName(button.outlineName);
        if (solidTex != nullptr || outlineTex != nullptr) {
            // InputViewer look: outline while idle, solid artwork on press.
            ImTextureID idle = outlineTex != nullptr ? outlineTex : solidTex;
            sprite(idle, button, button.uv0, button.uv1, button.pressed ? 1.0f : 0.9f);
            if (button.pressed && solidTex != nullptr) {
                sprite(solidTex, button, button.uv0, button.uv1, 1.0f);
            }
        } else {
            // Vector fallback (textures unavailable): colored disc + glyph.
            const float r = std::min(button.halfW, button.halfH);
            const ImU32 fill = button.pressed ? (rgb(button.color) | alpha(1.0f)) : (rgb(button.color) | alpha(0.55f));
            drawList->AddCircleFilled(button.center, r, fill);
            drawList->AddCircle(button.center, r, IM_COL32(255, 255, 255, 0) | alpha(0.9f), 0, stroke);
            if (button.dirX != 0 || button.dirY != 0) {
                const float a = r * 0.5f;
                const ImVec2 tip(button.center.x + button.dirX * a, button.center.y + button.dirY * a);
                const ImVec2 b1(button.center.x - button.dirX * a * 0.6f + button.dirY * a,
                                button.center.y - button.dirY * a * 0.6f + button.dirX * a);
                const ImVec2 b2(button.center.x - button.dirX * a * 0.6f - button.dirY * a,
                                button.center.y - button.dirY * a * 0.6f - button.dirX * a);
                drawList->AddTriangleFilled(tip, b1, b2, IM_COL32(20, 20, 26, 0) | alpha(0.9f));
            } else {
                text(button.center, r * 1.2f, button.label, IM_COL32(255, 255, 255, 0) | alpha(0.95f));
            }
        }
    }

    // Stick: AnalogStickOutline is the housing/gate ring (base), AnalogStick is
    // the cap (deflected position); circles are the no-texture fallback.
    const ImVec2 base = sState.stickHeld ? sState.stickAnchor : sState.stickRest;
    const ImVec2 nub = sState.stickHeld ? sState.stickPos : sState.stickRest;
    const float baseAlpha = sState.stickHeld ? 0.85f : 0.55f;
    ImTextureID capTex = gui->GetTextureByName("Analog-Stick");
    ImTextureID gateTex = gui->GetTextureByName("Analog-Stick Outline");
    // Housing/gate crop within the 327x175 frame (ring around the cap's spot).
    const ImVec2 gateUv0(18.0f / kFrameW, 40.0f / kFrameH);
    const ImVec2 gateUv1(116.0f / kFrameW, 138.0f / kFrameH);
    // Sized so the cap touches the gate ring at full deflection.
    const float gateHalf = sState.stickTravel + sState.nubRadius;
    if (gateTex != nullptr) {
        drawList->AddImage(gateTex, ImVec2(base.x - gateHalf, base.y - gateHalf),
                           ImVec2(base.x + gateHalf, base.y + gateHalf), gateUv0, gateUv1,
                           IM_COL32(255, 255, 255, 0) | alpha(baseAlpha));
    } else {
        drawList->AddCircleFilled(base, sState.stickTravel, IM_COL32(20, 20, 26, 0) | alpha(0.22f * baseAlpha));
        drawList->AddCircle(base, sState.stickTravel, IM_COL32(255, 255, 255, 0) | alpha(baseAlpha), 0, stroke);
    }
    if (capTex != nullptr) {
        drawList->AddImage(capTex, ImVec2(nub.x - sState.nubRadius, nub.y - sState.nubRadius),
                           ImVec2(nub.x + sState.nubRadius, nub.y + sState.nubRadius), sState.stickUv0, sState.stickUv1,
                           IM_COL32(255, 255, 255, 0) | alpha(baseAlpha + 0.15f));
    } else {
        drawList->AddCircleFilled(nub, sState.nubRadius, IM_COL32(190, 190, 200, 0) | alpha(baseAlpha));
        drawList->AddCircle(nub, sState.nubRadius, IM_COL32(255, 255, 255, 0) | alpha(baseAlpha + 0.15f), 0, stroke);
    }

    // Layout-edit chrome: highlight the dragged widget, Done/Reset pills, hint.
    if (sState.editMode) {
        if (sDragTarget >= 0 && sDragTarget < (int)sState.gameButtons.size()) {
            const TouchButton& b = sState.gameButtons[sDragTarget];
            drawList->AddRect(ImVec2(b.center.x - b.halfW - u * 0.3f, b.center.y - b.halfH - u * 0.3f),
                              ImVec2(b.center.x + b.halfW + u * 0.3f, b.center.y + b.halfH + u * 0.3f),
                              IM_COL32(255, 255, 255, 220), u * 0.2f, 0, stroke);
        } else if (sDragTarget == -2) {
            drawList->AddCircle(sState.stickRest, sState.stickTravel + u * 0.3f, IM_COL32(255, 255, 255, 220), 0,
                                stroke);
        } else if (sDragTarget == -3) {
            drawList->AddCircle(sState.menuCenter, sState.menuRadius + u * 0.3f, IM_COL32(255, 255, 255, 220), 0,
                                stroke);
        }

        const auto pill = [&](const ImVec2& c, const ImVec2& half, ImU32 fillColor, const char* s, bool down) {
            const ImVec2 min(c.x - half.x, c.y - half.y);
            const ImVec2 max(c.x + half.x, c.y + half.y);
            drawList->AddRectFilled(min, max, (fillColor & ~IM_COL32_A_MASK) | (down ? IM_COL32_A_MASK : alpha(0.85f)),
                                    half.y);
            drawList->AddRect(min, max, IM_COL32(255, 255, 255, 230), half.y, 0, stroke);
            text(c, half.y * 1.0f, s, IM_COL32(255, 255, 255, 255));
        };
        pill(sState.doneCenter, sState.doneHalf, IM_COL32(40, 170, 80, 255), "Done", sDoneDown);
        pill(sState.resetCenter, sState.resetHalf, IM_COL32(120, 120, 130, 255), "Reset", sResetDown);

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        text(ImVec2(display.x * 0.5f, 3.6f * u), u * 0.9f, "Drag controls to reposition them",
             IM_COL32(255, 255, 255, 200));
    }
}

} // namespace GhostshipGui
