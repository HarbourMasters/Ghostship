#include "ObjectBehavior.h"
#include "port/Rando/Logic/Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

struct ObjectHitbox randoRedCoinHitbox = {
    /* interactType:      */ (1 << 4),
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 2,
    /* health:            */ 0,
    /* numLootCoins:      */ 0,
    /* radius:            */ 100,
    /* height:            */ 64,
    /* hurtboxRadius:     */ 0,
    /* hurtboxHeight:     */ 0,
};

static struct ObjectHitbox randoCollectStarHitbox = {
    /* interactType:      */ (1 << 12),
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 0,
    /* health:            */ 0,
    /* numLootCoins:      */ 0,
    /* radius:            */ 80,
    /* height:            */ 50,
    /* hurtboxRadius:     */ 0,
    /* hurtboxHeight:     */ 0,
};

void Rando::ObjectBehavior::ModifyBlueCoinSwitchBehavior() {
    for (auto& [randoCheckId, object] : spawnedRandoObjects) {
        if (Rando::Logic::IsBlueSwitchActivated(randoCheckId)) {
            object->header.gfx.node.flags |= GRAPH_RENDER_ACTIVE;
            object->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
            object->oIntangibleTimer = 0;

            if (object->behavior == bhvRedCoin) {
                obj_set_hitbox(object, &randoRedCoinHitbox);
            }
            if (object->behavior == bhvStar) {
                obj_set_hitbox(object, &randoCollectStarHitbox);
            }
        }
    }
}