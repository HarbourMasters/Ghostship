#include "ObjectBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "game/interaction.h"
#include "port/features/Features.h"
}

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

void Rando::ObjectBehavior::ModifyStarBehavior(bool* shouldCancel, struct Object* obj) {
    if (obj->unused1 != RC_UNKNOWN) {
        if (Rando::Logic::IsBlueSwitchActivated((RandoCheckId)obj->unused1)) {
            for (auto& spawned : spawnedRandoObjects) {
                if (spawned.second == obj) {
                    obj->header.gfx.node.flags =
                        spawnedRandoObjects.at((RandoCheckId)obj->unused1)->header.gfx.node.flags;
                    break;
                }
            }

            obj->oIntangibleTimer = -1;
            if (Port_FeatureInt("enhancements.star_no_exit", 0)) {
                obj->oInteractionSubtype |= INT_SUBTYPE_NO_EXIT;
            }
            *(shouldCancel) = true;
            return;
        }
    }
    *(shouldCancel) = false;
}