#include "mods.h"
#include <libultraship/bridge.h>
#include "game-interactor/GameInteractor.h"

#include "game/level_update.h"
#include "sm64.h"
extern "C" {
#include "game/mario.h"
extern MarioState* gMarioState;
}

#define MARIO_HEALTH_MAX 0x880

void RegisterInfiniteHealth() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnHealthChange>([](int16_t health) {
        if (!gMarioState) return;

        if (CVarGetInteger("gInfiniteHealth", 0) != 0) {
            gMarioState->health = MARIO_HEALTH_MAX;
        }
    });
}

void RegisterInfiniteLives() {
    // There may be a better hook that can be made, rather than doing this every frame
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (!gMarioState) return;

        if (CVarGetInteger("gInfiniteLives", 0) != 0) {
            gMarioState->numLives = 100;
        }
    });
}

void RegisterFlightVelocityBoost() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (!gMarioState) return;

        if (CVarGetInteger("gFlightVelocityBoost", 0) && gMarioState->input & INPUT_A_DOWN) {
            mario_set_forward_vel(gMarioState, gMarioState->forwardVel + 1.0f);
        }
    });
}

void InitMods() {
    RegisterInfiniteHealth();
    RegisterInfiniteLives();
    RegisterFlightVelocityBoost();
}
