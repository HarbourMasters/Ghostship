#pragma once

#include <ship/window/gui/GuiWindow.h>

class AchievementsWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~AchievementsWindow() = default;

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};