#pragma once

#define CVAR_TOUCH(var) "gTouchControls." var

// Merges the on-screen touch controls into the game's pad buffer; called after
// osContGetReadData(). `pads` is OSContPad[MAXCONTROLLERS]; only port 0 is written.
#ifdef __cplusplus
extern "C" void TouchControls_ApplyPad(void* pads);
#else
void TouchControls_ApplyPad(void* pads);
#endif

#ifdef __cplusplus
#include <libultraship/libultraship.h>

namespace GhostshipGui {

// Virtual-gamepad overlay. Input math lives in TouchControls_ApplyPad() on the
// game tick; Draw() only renders the state computed there.
class TouchControlsOverlay final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void Draw() override;
    void InitElement() override;
    void DrawElement() override {};
    void UpdateElement() override {};
};

} // namespace GhostshipGui
#endif
