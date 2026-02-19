#pragma once

#include "UIWidgets.hpp"
#include "InputViewer.h"
#include "GhostshipModals.h"
#include "GhostshipMenu.h"

namespace GhostshipGui {
void SetupHooks();
void SetupMenu();
void SetupGuiElements();
void Draw();
void Destroy();
void RegisterPopup(std::string title, std::string message, std::string button1 = "OK", std::string button2 = "",
                   std::function<void()> button1callback = nullptr, std::function<void()> button2callback = nullptr);
size_t PopupsQueued();
UIWidgets::Colors GetMenuThemeColor();
extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
} // namespace GhostshipGui

#define THEME_COLOR GhostshipGui::GetMenuThemeColor()
