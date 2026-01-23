#ifndef RANDO_LOGIC_H
#define RANDO_LOGIC_H

#include "port/Rando/Rando.h"
#include "port/ShipUtils.h"

struct LevelShuffleEntry {
    RandoCheckId randoCheckId;
    RandoItemId randoItemId;
    RandoAct randoAct;
};

namespace Rando {

namespace Logic {

extern std::vector<std::vector<LevelShuffleEntry>> shuffledList;
extern std::vector<LevelShuffleEntry> shuffledLevelList;
extern std::vector<RandoCheckId> shuffledChecks;
extern std::vector<std::pair<RandoItemId, RandoAct>> shuffledItems;

void shuffleRandoItems(std::vector<std::pair<RandoItemId, RandoAct>>& shuffledItems);
void GenerateShuffleList();

// Logic Operators
inline bool IsBlueSwitchActivated(RandoCheckId randoCheckId) {
    if (Rando::StaticData::Checks[randoCheckId].randoItemId == RI_COIN_BLUE) {
        return true;
    }
    return false;
};

inline bool IsCheckShuffled(int16_t levelId, RandoCheckId randoCheckId) {
    if (levelId < 0 || levelId >= shuffledList.size()) {
        return false;
    }

    for (auto& entry : shuffledList[levelId]) {
        if (entry.randoCheckId == randoCheckId) {
            return true;
        }
    }
    return false;
}


} // namespace Logic

} // namespace Rando

#endif // RANDO_LOGIC_H