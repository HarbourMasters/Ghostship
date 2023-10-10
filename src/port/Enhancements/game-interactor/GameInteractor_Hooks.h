#include "GameInteractor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gameplay
void GameInteractor_ExecuteOnGameFrameUpdate();
void GameInteractor_ExecuteOnHealthChange(int16_t health);

#ifdef __cplusplus
}
#endif
