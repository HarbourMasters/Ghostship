#pragma once

#include <libultraship/libultraship.h>
#include "UIWidgets.hpp"
#include "Menu.h"
#include <fast/backends/gfx_rendering_api.h>
#include "port/ui/cvar_prefixes.h"
#include "port/ui/enhancementTypes.h"

namespace GhostshipGui {
class GhostshipMenu : public Ship::Menu {
  public:
    GhostshipMenu(const std::string& consoleVariable, const std::string& name);

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
    void Draw() override;

    void AddSidebarEntry(std::string sectionName, std::string sidbarName, uint32_t columnCount);
    WidgetInfo& AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType);
    void AddMenuSettings();
    void AddMenuEnhancements();

  private:
    char mGitCommitHashTruncated[8];
    bool mIsTaggedVersion;
};
} // namespace GhostshipGui
