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
            obj->header.gfx.node.flags = spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags;
            obj->oIntangibleTimer = -1;
            if (CVarGetInteger("gEnhancements.StarNoExit", 0)) {
                obj->oInteractionSubtype |= INT_SUBTYPE_NO_EXIT;
            }
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}