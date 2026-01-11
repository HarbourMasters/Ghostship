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
    RI(RI_UNKNOWN,  "", "Unknown",  RITYPE_UNKNOWN, MODEL_NONE),
    RI(RI_COIN_YELLOW, "", "Yellow Coin", RITYPE_COIN, MODEL_YELLOW_COIN),
    RI(RI_SIGNPOST, "", "Signpost", RITYPE_SIGNPOST, MODEL_WOODEN_SIGNPOST),
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

RandoItemId GetItemIdFromName(const char* name) {
    for (auto& [randoItemId, randoStaticItem] : Items) {
        if (strcmp(name, randoStaticItem.spoilerName) == 0) {
            return randoItemId;
        }
    }
    return RI_UNKNOWN;
}

} // namespace StaticData
} // namespace Rando