#include "Logic.h"

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {

void ApplyNoLogicToSaveContext(std::vector<std::vector<LevelShuffleEntry>>& initialPool) {
    std::vector<LevelShuffleEntry> pool;
    std::vector<RandoCheckId> checkPool;
    std::vector<std::pair<RandoItemId, RandoAct>> itemPool;

    for (auto& level : initialPool) {
        checkPool.clear();
        itemPool.clear();

        for (auto& entry : level) {
            checkPool.push_back(entry.randoCheckId);
            itemPool.push_back({ entry.randoItemId, entry.randoAct });
        }

        if (!itemPool.empty()) {
            ShuffleRandoItems(itemPool);
            for (int v = 0; v < checkPool.size(); v++) {
                pool.push_back({ checkPool[v], itemPool[v].first, itemPool[v].second, false });
            }
        }
    }
    shuffledPool = pool;
    initialPool.clear();
}

} // namespace Logic

} // namespace Rando