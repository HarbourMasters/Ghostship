#include "Logic.h"

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {

void ApplyNoLogicToSaveContext(std::vector<std::vector<LevelShuffleEntry>>& initialPool,
                               std::vector<int16_t>& initialLevelPool) {
    std::vector<LevelShuffleEntry> levelpool;
    std::vector<RandoCheckId> checkPool;
    std::vector<std::pair<RandoItemId, RandoAct>> itemPool;

    std::vector<RandoSaveEntrance> entrancePool;

    if (!initialPool.empty()) {
        for (auto& level : initialPool) {
            checkPool.clear();
            itemPool.clear();

            for (auto& entry : level) {
                checkPool.push_back(entry.randoCheckId);
                itemPool.push_back({ entry.randoItemId, entry.randoAct });
            }

            if (!itemPool.empty()) {
                ShuffleRandoItems(itemPool, seedString);
                for (int v = 0; v < checkPool.size(); v++) {
                    levelpool.push_back({ checkPool[v], itemPool[v].first, itemPool[v].second, false });
                }
            }
        }
    }

    if (!initialLevelPool.empty()) {
        ShuffleRandoEntrances(initialLevelPool, seedString);
        for (int e = 0; e < initialLevelPool.size(); e++) {
            RandoSaveEntrance randoSaveEntrance;
            randoSaveEntrance.randoEntranceId = entranceIds[e];
            randoSaveEntrance.destinationId = initialLevelPool[e];
            entrancePool.push_back(randoSaveEntrance);
        }
    }

    shuffledPool = levelpool;
    shuffledEntrances = entrancePool;
    initialPool.clear();
    initialLevelPool.clear();
}

} // namespace Logic

} // namespace Rando