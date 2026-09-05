#include "GhostshipMenu.h"

#include <ship/window/gui/ShaderSettingsWindow.h>

namespace GhostshipGui {

using namespace UIWidgets;

void GhostshipMenu::AddMenuShaderSettings() {
    AddMenuEntry("Shaders", CVAR_SETTING("Menu.ShadersSidebarSection"));

    AddSidebarEntry("Shaders", "Shader Packs", 1);
    WidgetPath path = { "Shaders", "Shader Packs", SECTION_COLUMN_1 };

    AddWidget(path, "Popout Shader Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ShaderSettings"))
        .WindowName("Shader Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Tweakables for shader packs: post-processing passes and @setting "
                                               "values declared by custom shaders."));
}

} // namespace GhostshipGui