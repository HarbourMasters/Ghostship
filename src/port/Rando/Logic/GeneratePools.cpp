#include "Logic.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/ui/Notification.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <sstream>
#include <random>

extern "C" {
#include "port/ShipUtils.h"
}

namespace Rando {

namespace Logic {
// Initial Shuffling containers
std::vector<std::vector<LevelShuffleEntry>> shuffledList;
std::vector<LevelShuffleEntry> shuffledLevelList;
std::vector<RandoCheckId> shuffledChecks;
std::vector<std::pair<RandoItemId, RandoAct>> shuffledItems;

// Final Shuffle List
std::vector<LevelShuffleEntry> shuffledPool;

void ShuffleRandoItems(std::vector<std::pair<RandoItemId, RandoAct>>& shuffledItems) {
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(shuffledItems.begin(), shuffledItems.end(), g);
}

void GenerateShuffleList() {
    shuffledPool.clear();

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

            if (randoCheckData.randoCheckType == RCTYPE_STAR_RED_COIN &&
                CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_RED_COIN_STARS].cvar, 0) == RO_GENERIC_OFF) {
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
            for (int v = 0; v < shuffledChecks.size(); v++) {
                shuffledLevelList.push_back(
                    { shuffledChecks[v], shuffledItems[v].first, shuffledItems[v].second, false });
            }
            shuffledList.push_back(shuffledLevelList);
        }
    }

    switch (CVarGetInteger(Rando::StaticData::Options[RO_LOGIC].cvar, 0)) {
        case RO_LOGIC_GLITCHLESS:
            break;
        case RO_LOGIC_NO_LOGIC:
            ApplyNoLogicToSaveContext(shuffledList);
            break;
        default:
            break;
    }

    if (CVarGetInteger("gRandoSettings.GenerateLog", 0)) {
        nlohmann::json spoilerLog = Rando::Spoiler::GenerateFromPoolGeneration(shuffledPool);
        if (spoilerLog.empty()) {
            Notification::Emit(
                { .message = "Error: No Spoiler Log was created.", .messageColor = ImVec4(0.85f, 0.3f, 0, 1) });
        } else {
            std::string fileName = spoilerLog["fileNum"].get<std::string>() + ".json";
            Rando::Spoiler::SaveToFile(fileName, spoilerLog);
            Notification::Emit({ .prefix = fileName + " ",
                                 .message = "Spoiler Log created.",
                                 .messageColor = ImVec4(0, 0.3f, 0.85f, 1) });
        }
    }
}

} // namespace Logic

} // namespace Rando