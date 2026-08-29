#pragma once

#include <ship/window/gui/GuiWindow.h>

class ObjectViewer : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~ObjectViewer() = default;

    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;
    void UpdateElement() override;
    void DrawElement() override;
};
