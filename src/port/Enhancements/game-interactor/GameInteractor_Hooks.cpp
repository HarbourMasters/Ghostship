#include "GameInteractor_Hooks.h"

// Gameplay

// Updates only when in game, i.e. past the save select screen
void GameInteractor_ExecuteOnGameFrameUpdate() {
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnGameFrameUpdate>();
}

void GameInteractor_ExecuteOnHealthChange(int16_t health) {
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnHealthChange>(health);
}
