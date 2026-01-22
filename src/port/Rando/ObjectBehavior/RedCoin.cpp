#include "ObjectBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyRedCoinBehavior(bool* shouldCancel, struct Object* obj) {
    if (obj->unused1 != RC_UNKNOWN) {
        if (Rando::Logic::IsBlueSwitchActivated((RandoCheckId)obj->unused1)) {
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->oIntangibleTimer = -1;
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}