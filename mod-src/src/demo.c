#include "mod.h"

#include "port/events/Events.h"
#include "game/level_update.h"
#include "sm64.h"
#include "game/print.h"
#include <stdio.h>
#include "port/api/ui.h"

ListenerID gFrameUpdateListenerID;
ListenerID gRenderGamePostListenerID;

void SetupUI() {
    C_WidgetConfig chk = {0};
    chk.type = C_WIDGET_CVAR_CHECKBOX;
    chk.cvar = "gSkipIntro";
    chk.opts.checkbox.tooltip = "Skip the intro cutscene.";
    chk.opts.checkbox.default_val = true;
    C_AddSidebarEntry("My Mod", 1);
    C_AddWidget("My Mod", 1, "Draw Mod Hi", &chk);
}

void OnFrameUpdate(IEvent* event) {
    gMarioState->numCoins = 99;
}

void OnGameRenderHud(IEvent* event) {
    if (CVarGetInteger("gSkipIntro", 0) == 0) {
        return;
    }

    print_text_centered(160, 80, "MOD HI");
}

MOD_INIT() {
    SetupUI();
    gFrameUpdateListenerID = REGISTER_LISTENER(GameFrameUpdate, EVENT_PRIORITY_NORMAL, OnFrameUpdate);
    gRenderGamePostListenerID = REGISTER_LISTENER(RenderGamePost, EVENT_PRIORITY_NORMAL, OnGameRenderHud);
}

MOD_EXIT() {
    C_RemoveSidebarEntry("My Mod");

    UNREGISTER_LISTENER(GameFrameUpdate, gFrameUpdateListenerID);
    UNREGISTER_LISTENER(RenderGamePost, gRenderGamePostListenerID);
}