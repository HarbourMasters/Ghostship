#include "mods.h"
#include <libultraship/bridge.h>
#include "game-interactor/GameInteractor.h"

#include "game/level_update.h"
#include "include/sm64.h"
extern MarioState* gMarioState;

#define MARIO_HEALTH_MAX 0x880

void RegisterCheatsHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnHealthChange>([](int16_t health) {
        if (!gMarioState) return;

        if (CVarGetInteger("gInfiniteHealth", 0) != 0) {
            gMarioState->health = MARIO_HEALTH_MAX;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (!gMarioState) return;

        // There may be a better hook that can be made, rather than doing this every frame
        if (CVarGetInteger("gInfiniteLives", 0) != 0) {
            gMarioState->numLives = 100;
        }

        if (CVarGetInteger("gInfiniteCapTimer", 0) && (gMarioState->flags & MARIO_CAP_ON_HEAD) && (gMarioState->flags & MARIO_SPECIAL_CAPS)) {
            gMarioState->capTimer = 1200;
        }
    });
}

void InitMods() {
    RegisterCheatsHooks();
}
