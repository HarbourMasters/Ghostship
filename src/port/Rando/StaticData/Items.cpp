#include "StaticData.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "port/ShipUtils.h"
#include "port/Rando/Rando.h"

#include "include/macro_presets.h"

namespace Rando {

namespace StaticData {

#define RI(id, article, name, type, itemId)      \
    {                                            \
        id, {                                    \
            id, #id, article, name, type, itemId \
        }                                        \
    }

// clang-format off
std::map<RandoItemId, RandoStaticItem> Items = {
    RI(RI_UNKNOWN,      "", "Unknown",      RITYPE_UNKNOWN,     MODEL_NONE),
    RI(RI_COIN_BLUE,    "", "Blue Coin",    RITYPE_COIN_BLUE,   MODEL_BLUE_COIN),
    RI(RI_COIN_RED,     "", "Red Coin",     RITYPE_COIN_RED,    MODEL_RED_COIN),
    RI(RI_STAR,         "", "Star",         RITYPE_STAR,        MODEL_STAR),
};
// clang-format on

int16_t GetModelByRandoItem(RandoItemId randoItem) {
    for (auto& [randoItemId, randoStaticItem] : Rando::StaticData::Items) {
        if (randoItemId == randoItem) {
            return randoStaticItem.modelId;
        }
    }
    return NULL;
}

const BehaviorScript* GetBehaviorByModel(int16_t modelId) {
    if (modelId == MODEL_STAR) {
        return bhvStar;
    }
    for (auto& macro : MacroObjectPresets) {
        if (macro.model == modelId) {
            return macro.behavior;
        }
    }
    return nullptr;
}

RandoItemId GetShuffledRandoItem(RandoCheckId randoCheckId) {
    for (auto& [randoCheck, randoItem] : Rando::Logic::shuffledList) {
        if (randoCheck == randoCheckId) {
            return randoItem;
        }
    }
    return RI_UNKNOWN;
}

} // namespace StaticData
} // namespace Rando