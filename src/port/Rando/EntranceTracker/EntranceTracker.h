#ifndef RANDO_ENTRANCE_TRACKER_H
#define RANDO_ENTRANCE_TRACKER_H

#include "port/Rando/Rando.h"
#include <ship/window/gui/GuiWindow.h>

namespace Rando {

namespace EntranceTracker {

void Init();

class EntranceTrackerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override{};
    void Draw() override;
    void UpdateElement() override{};
};

class SettingsWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

} // namespace EntranceTracker

} // namespace Rando

#endif // RANDO_ENTRANCE_TRACKER_H