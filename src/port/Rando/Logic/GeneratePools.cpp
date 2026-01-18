#include "Logic.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <sstream>
#include <random>

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {

std::map<RandoCheckId, RandoItemId> shuffledList;
std::vector<RandoCheckId> shuffledChecks;
std::vector<RandoItemId> shuffledItems;

void shuffleRandoItems(std::vector<RandoItemId>& shuffledItems) {
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(shuffledItems.begin(), shuffledItems.end(), g);
}

void GenerateShuffleList() {
    Rando::Logic::shuffledList.clear();

    for (auto& [randoCheckId, randoCheckData] : Rando::StaticData::Checks) {
        if (randoCheckId == RC_UNKNOWN) {
            continue;
        }

        // TODO: Temporary for testing Stars.
        if (randoCheckData.levelId != LEVEL_WF) {
            continue;
        }

        RandoItemType randoItemType = Rando::StaticData::Items[randoCheckData.randoItemId].randoItemType;

        // TODO: Swap to RANDO_SAVE_OPTIONS once Save File is converted to JSON
        if (randoItemType == RITYPE_STAR &&
            CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_STARS].cvar, 0) == RO_GENERIC_OFF) {
            continue;
        }

        if (randoItemType == RITYPE_COIN_BLUE &&
            CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_COINS_BLUE].cvar, 0) == RO_GENERIC_OFF) {
            continue;
        }

        if (randoItemType == RITYPE_COIN_RED &&
            CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_COINS_RED].cvar, 0) == RO_GENERIC_OFF) {
            continue;
        }

        shuffledChecks.push_back(randoCheckId);
        shuffledItems.push_back(randoCheckData.randoItemId);
    }

    shuffleRandoItems(shuffledItems);
    for (int i = 0; i < shuffledChecks.size(); i++) {
        Rando::Logic::shuffledList.insert({ shuffledChecks[i], shuffledItems[i] });
    }
}

} // namespace Logic

} // namespace Rando