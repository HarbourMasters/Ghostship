#pragma once

#include "UIWidgets.hpp"
#include "InputViewer.h"
#include "GhostshipModals.h"

namespace GhostshipGui {
void SetupHooks();
void SetupGuiElements();
void Draw();
void Destroy();
UIWidgets::Colors GetMenuThemeColor();
} // namespace GhostshipGui

#define THEME_COLOR GhostshipGui::GetMenuThemeColor()
