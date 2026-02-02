#ifndef RANDO_LOGIC_H
#define RANDO_LOGIC_H

#include "port/Rando/Rando.h"
#include "port/ShipUtils.h"

struct LevelShuffleEntry {
    RandoCheckId randoCheckId;
    RandoItemId randoItemId;
    RandoAct randoAct;
    bool obtained;
    bool skipped;
};

namespace Rando {

namespace Logic {

// Initial Check Shuffling containers
extern std::vector<std::vector<LevelShuffleEntry>> shuffledList;
extern std::vector<LevelShuffleEntry> shuffledLevelList;
extern std::vector<RandoCheckId> shuffledChecks;
extern std::vector<std::pair<RandoItemId, RandoAct>> shuffledItems;

// Initial Entrance Shuffling containers
extern std::vector<RandoEntranceId> entranceIds;
extern std::vector<int16_t> levelIds;

// Final Shuffle List
extern std::vector<LevelShuffleEntry> shuffledPool;
extern std::vector<RandoSaveEntrance> shuffledEntrances;

void ShuffleRandoItems(std::vector<std::pair<RandoItemId, RandoAct>>& shuffledItems, const std::string& input);
void ShuffleRandoEntrances(std::vector<int16_t>& shuffledLevels, const std::string& input);
void InitializeSaveChecks();
void InitializeSaveEntrances();
void InitializeSaveOptions();
void GenerateShuffleList();

void ApplyNoLogicToSaveContext(std::vector<std::vector<LevelShuffleEntry>>& initialPool,
                               std::vector<int16_t>& initialLevelPool);

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