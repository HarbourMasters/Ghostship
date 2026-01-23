#ifndef RANDO_CHECK_TRACKER_H
#define RANDO_CHECK_TRACKER_H

#include "port/Rando/Rando.h"
#include <ship/window/gui/GuiWindow.h>

typedef struct {
    int16_t levelId;
    std::string levelName;
    std::vector<std::tuple<RandoCheckId, std::string, bool>> randoCheckNameList;
} CheckTrackerObject;

extern std::vector<CheckTrackerObject> checkTrackerList;

namespace Rando {

namespace CheckTracker {

void Init();
void OnFileLoad();

class CheckTrackerWindow : public Ship::GuiWindow {
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

} // namespace CheckTracker

} // namespace Rando

#endif // RANDO_CHECK_TRACKER_H