#include "ObjectBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "game/interaction.h"
}

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyStarBehavior(bool* shouldCancel, struct Object* obj) {
    if (obj->unused1 != RC_UNKNOWN) {
        if (Rando::Logic::IsBlueSwitchActivated((RandoCheckId)obj->unused1)) {
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
            spawnedRandoObjects.at((RandoCheckId)obj->unused1)->oIntangibleTimer = -1;
            if (CVarGetInteger("gEnhancements.StarNoExit", 0)) {
                spawnedRandoObjects.at((RandoCheckId)obj->unused1)->oInteractionSubtype |= INT_SUBTYPE_NO_EXIT;
            }
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}