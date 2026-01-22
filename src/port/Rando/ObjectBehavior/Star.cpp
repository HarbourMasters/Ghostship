#include "ObjectBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyStarBehavior(bool* shouldCancel, struct Object* obj) {
    if (obj->unused1 != RC_UNKNOWN) {
        if (obj->unused1 >= RC_WF_BLUE_COIN_01 && obj->unused1 <= RC_WF_BLUE_COIN_04) {
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->oIntangibleTimer = -1;
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}