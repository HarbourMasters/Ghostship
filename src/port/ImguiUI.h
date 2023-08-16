#pragma once
#include <libultraship/libultraship.h>

namespace GameUI {
    void SetupGuiElements();
    void Destroy();
}

class GameMenuBar : public LUS::GuiMenuBar {
  public:
    using LUS::GuiMenuBar::GuiMenuBar;
  protected:
    void DrawElement() override;
    void InitElement() override {};
    void UpdateElement() override {};
};