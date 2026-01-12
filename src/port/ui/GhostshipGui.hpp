#pragma once

#include "UIWidgets.hpp"
#include "InputViewer.h"
#include "GhostshipModals.h"
#include "GhostshipMenu.h"

namespace GhostshipGui {
void SetupHooks();
void SetupGuiElements();
void Draw();
void Destroy();
UIWidgets::Colors GetMenuThemeColor();
extern std::shared_ptr<GhostshipMenu> mGhostshipMenu;
} // namespace GhostshipGui

#define THEME_COLOR GhostshipGui::GetMenuThemeColor()
