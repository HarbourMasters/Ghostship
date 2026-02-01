#include "ObjectBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyRedCoinBehavior(bool* shouldCancel, struct Object* obj) {
    if (obj->unused1 != RC_UNKNOWN) {
        if (Rando::Logic::IsBlueSwitchActivated((RandoCheckId)obj->unused1)) {
            obj->header.gfx.node.flags = spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags;
            obj->oIntangibleTimer = -1;
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}