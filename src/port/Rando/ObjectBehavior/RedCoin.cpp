#include "ObjectBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyRedCoinBehavior() {
    for (auto& [randoCheckId, object] : spawnedRandoObjects) {
        if (randoCheckId >= RC_WF_BLUE_COIN_01 && randoCheckId <= RC_WF_BLUE_COIN_04) {
            object->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
            object->oIntangibleTimer = -1;
            break;
        }
    }
}