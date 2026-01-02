#include "PortEnhancements.h"
#include "sm64.h"
#include "game/level_update.h"

#define INIT_EVENT_IDS
#include "port/hooks/Events.h"

#define MARIO_HEALTH_MAX 0x880

void OnGameUpdate(IEvent* event) {
    if(gMarioState == NULL) {
        return;
    }

    if (CVarGetInteger("gInfiniteLives", 0) != 0) {
        gMarioState->numLives = MARIO_HEALTH_MAX;
    }
}

void OnHealthChange(IEvent* event) {
    if (CVarGetInteger("gInfiniteHealth", 0) != 0) {
        gMarioState->health = MARIO_HEALTH_MAX;
    }
}

void PortEnhancements_Init() {
    PortEnhancements_Register();

    // Register event listeners
    REGISTER_LISTENER(GameFrameUpdate, OnGameUpdate, EVENT_PRIORITY_NORMAL);
    REGISTER_LISTENER(PlayerHealthChange, OnHealthChange, EVENT_PRIORITY_NORMAL);
}

void PortEnhancements_Register() {
    // Register engine events
    REGISTER_EVENT(GameFrameUpdate);
    REGISTER_EVENT(PlayerHealthChange);
}

void PortEnhancements_Exit() {
    // TODO: Unregister event listeners
}
