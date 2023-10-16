#include "GameInteractor.h"

#include "game/level_update.h"

// Taken from game/level_update.c
#define PLAY_MODE_NORMAL 0
#define PLAY_MODE_PAUSED 2
#define PLAY_MODE_CHANGE_AREA 3
#define PLAY_MODE_CHANGE_LEVEL 4
#define PLAY_MODE_FRAME_ADVANCE 5

bool GameInteractor::IsGameplayPaused() {
    return sCurrPlayMode == PLAY_MODE_PAUSED;
}
