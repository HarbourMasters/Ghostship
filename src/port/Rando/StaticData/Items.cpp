#include "StaticData.h"
#include <random>
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

static std::map<RandoCheckId, RandoItemId> shuffledList;

void shuffleRandoItems(std::vector<RandoItemId>& shuffledItems) {
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(shuffledItems.begin(), shuffledItems.end(), g);
}

// clang-format off
std::map<RandoItemId, RandoStaticItem> Items = {
    RI(RI_UNKNOWN,      "", "Unknown",      RITYPE_UNKNOWN, MODEL_NONE),
    RI(RI_COIN_RED,     "", "Red Coin",     RITYPE_COIN,    MODEL_RED_COIN),
    RI(RI_STAR,         "", "Star",         RITYPE_STAR,    MODEL_STAR),
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

const BehaviorScript *GetBehaviorByModel(int16_t modelId) {
    for (auto& macro : MacroObjectPresets) {
        if (macro.model == modelId) {
            return macro.behavior;
        }
    }
    return nullptr;
}

RandoItemId GetShuffledRandoItem(RandoCheckId randoCheckId) {
    for (auto& [randoCheck, randoItem] : Rando::StaticData::shuffledList) {
        if (randoCheck == randoCheckId) {
            return randoItem;
        }
    }
    return RI_UNKNOWN;
}

void ShuffleItemList() {
    Rando::StaticData::shuffledList.clear();
    std::vector<RandoCheckId> shuffledChecks;
    std::vector<RandoItemId> shuffledItems;
    for (auto& [randoCheckId, randoStaticItem] : Rando::StaticData::Checks) {
        if (randoCheckId == RC_UNKNOWN) {
            continue;
        }
        shuffledChecks.push_back(randoCheckId);
        shuffledItems.push_back(randoStaticItem.randoItemId);
    }

    shuffleRandoItems(shuffledItems);
    for (int i = 0; i < shuffledChecks.size(); i++) {
        Rando::StaticData::shuffledList.insert({ shuffledChecks[i], shuffledItems[i] });
    }
}

} // namespace StaticData
} // namespace Rando