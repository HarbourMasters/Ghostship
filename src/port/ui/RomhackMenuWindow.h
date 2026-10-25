#pragma once

#include <ship/window/gui/GuiWindow.h>

// Settings > Romhacks: generate a romhack from a patched ROM, pick the one to play, and list the supported ones.
class RomhackMenuWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~RomhackMenuWindow() = default;

  protected:
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;
    void DrawElement() override;
    void UpdateElement() override {
    }
};
