#include "Logic.h"

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {

void ApplyNoLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool) {
    // for (size_t i = 0; i < itemPool.size(); i++) {
    //     std::swap(itemPool[i], itemPool[Ship_Random(0, itemPool.size() - 1)]);
    // }
    //
    // for (auto& randoCheckId : checkPool) {
    //     if (randoCheckId == RC_UNKNOWN) {
    //         continue;
    //     }
    //
    //     RANDO_SAVE_CHECKS[randoCheckId].shuffled = true;
    //     RANDO_SAVE_CHECKS[randoCheckId].randoItemId = itemPool.back();
    //     itemPool.pop_back();
    // }
}

} // namespace Logic

} // namespace Rando