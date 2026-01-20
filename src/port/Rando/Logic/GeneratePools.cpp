#include "Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <sstream>
#include <random>

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {

std::vector<std::vector<LevelShuffleEntry>> shuffledList;
std::vector<LevelShuffleEntry> shuffledLevelList;
std::vector<RandoCheckId> shuffledChecks;
std::vector<std::pair<RandoItemId, RandoAct>> shuffledItems;

void shuffleRandoItems(std::vector<std::pair<RandoItemId, RandoAct>>& shuffledItems) {
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(shuffledItems.begin(), shuffledItems.end(), g);
}

void GenerateShuffleList() {
    shuffledList.clear();
    for (int i = LEVEL_UNKNOWN_1; i < LEVEL_UNKNOWN_38; i++) {
        shuffledLevelList.clear();
        shuffledChecks.clear();
        shuffledItems.clear();
        for (auto& [randoCheckId, randoCheckData] : Rando::StaticData::Checks) {
            if (randoCheckId == RC_UNKNOWN) {
                continue;
            }

            if (randoCheckData.levelId != i) {
                continue;
            }

            RandoItemType randoItemType = Rando::StaticData::Items[randoCheckData.randoItemId].randoItemType;

            // TODO: Swap to RANDO_SAVE_OPTIONS once Save File is converted to JSON
            if (randoItemType == RITYPE_COIN_BLUE &&
                CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_COINS_BLUE].cvar, 0) == RO_GENERIC_OFF) {
                continue;
            }

            if (randoItemType == RITYPE_COIN_RED &&
                CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_COINS_RED].cvar, 0) == RO_GENERIC_OFF) {
                continue;
            }

            if (randoItemType == RITYPE_STAR &&
                CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_STARS].cvar, 0) == RO_GENERIC_OFF) {
                continue;
            }

            shuffledChecks.push_back(randoCheckId);
            shuffledItems.push_back({ randoCheckData.randoItemId, randoCheckData.actData });
        }

        if (!shuffledItems.empty()) {
            shuffleRandoItems(shuffledItems);
            for (int v = 0; v < shuffledChecks.size(); v++) {
                shuffledLevelList.push_back({ shuffledChecks[v], shuffledItems[v].first, shuffledItems[v].second });
            }
        }

        shuffledList.push_back(shuffledLevelList);
    }
}

} // namespace Logic

} // namespace Rando