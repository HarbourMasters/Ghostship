#include "mod.h"

#include "port/events/Events.h"
#include "game/level_update.h"
#include "sm64.h"
#include "game/print.h"
#include <stdio.h>

ListenerID gFrameUpdateListenerID;
ListenerID gRenderGamePostListenerID;

void OnFrameUpdate(IEvent* event) {
    gMarioState->numCoins = 99;
}

void OnGameRenderHud(IEvent* event) {
    print_text_centered(160, 80, "MOD HI");
}

MOD_INIT() {
    gFrameUpdateListenerID = REGISTER_LISTENER(GameFrameUpdate, EVENT_PRIORITY_NORMAL, OnFrameUpdate);
    gRenderGamePostListenerID = REGISTER_LISTENER(RenderGamePost, EVENT_PRIORITY_NORMAL, OnGameRenderHud);
}

MOD_EXIT() {
    UNREGISTER_LISTENER(GameFrameUpdate, gFrameUpdateListenerID);
    UNREGISTER_LISTENER(RenderGamePost, gRenderGamePostListenerID);
}