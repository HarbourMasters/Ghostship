#ifndef RANDO_LOGIC_H
#define RANDO_LOGIC_H

#include "port/Rando/Rando.h"
#include "port/ShipUtils.h"

struct LevelShuffleEntry {
    RandoCheckId randoCheckId;
    RandoItemId randoItemId;
    RandoAct randoAct;
    bool obtained;
};

namespace Rando {

namespace Logic {

extern std::vector<std::vector<LevelShuffleEntry>> shuffledList;
extern std::vector<LevelShuffleEntry> shuffledLevelList;

extern std::vector<LevelShuffleEntry> shuffledPool;
extern std::vector<RandoCheckId> shuffledChecks;
extern std::vector<std::pair<RandoItemId, RandoAct>> shuffledItems;

void ShuffleRandoItems(std::vector<std::pair<RandoItemId, RandoAct>>& shuffledItems);
void InitializeSaveChecks();
void GenerateShuffleList();

void ApplyNoLogicToSaveContext(std::vector<std::vector<LevelShuffleEntry>>& initialPool);

// Logic Operators
inline bool IsBlueSwitchActivated(RandoCheckId randoCheckId) {
    if (Rando::StaticData::Checks[randoCheckId].randoItemId == RI_COIN_BLUE) {
        return true;
    }
    return false;
};

inline bool IsCheckShuffled(RandoCheckId randoCheckId) {
    for (auto& entry : shuffledPool) {
        if (entry.randoCheckId == randoCheckId) {
            return true;
        }
    }
    return false;
}


} // namespace Logic

} // namespace Rando

#endif // RANDO_LOGIC_H