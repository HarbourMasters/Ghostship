#include "mod.h"

#include "port/hooks/Events.h"
#include "game/level_update.h"
#include "sm64.h"
#include "game/print.h"
#include <stdio.h>

void OnFrameUpdate(IEvent* event) {
    gMarioState->numCoins = 99;
}

void OnGameRenderHud(IEvent* event) {
    print_text_centered(160, 80, "MOD HI");
}

MOD_INIT() {
    REGISTER_LISTENER(GameFrameUpdate, EVENT_PRIORITY_NORMAL, OnFrameUpdate);
    REGISTER_LISTENER(RenderGamePost, EVENT_PRIORITY_NORMAL, OnGameRenderHud);
}