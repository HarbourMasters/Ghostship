#include "Spoiler.h"
#include "port/Rando/Logic/Logic.h"
#include "port/ui/Notification.h"

namespace Rando {

namespace Spoiler {
nlohmann::json GenerateFromPoolGeneration(std::vector<LevelShuffleEntry>& shuffledPool,
                                          std::vector<RandoSaveEntrance>& shuffledEntrances) {
    nlohmann::json spoiler;
    spoiler["type"] = "GHOSTSHIP_RANDO_SPOILER";
    if (CVarGetInteger("gRandoSettings.ManualSeedEntry", 0)) {
        spoiler["seed"] = seedString;
    } else {
        spoiler["seed"] = "";
    }
    spoiler["finalSeed"] = gSaveBuffer.files[selectedFileNum][0].shipSaveData.randoSaveData.finalSeed;

    spoiler["options"] = nlohmann::json::object();
    for (auto& [randoOptionId, randoStaticOption] : Rando::StaticData::Options) {
        spoiler["options"][randoStaticOption.name] = RANDO_SAVE_OPTIONS(selectedFileNum)[randoOptionId];
    }

    spoiler["checks"] = nlohmann::json::object();
    for (auto& entry : shuffledPool) {
        if (entry.randoAct != RA_ACT_NONE) {
            spoiler["checks"][Rando::StaticData::Checks[entry.randoCheckId].name] = nlohmann::json::object();
            spoiler["checks"][Rando::StaticData::Checks[entry.randoCheckId].name]["randoItemId"] =
                Rando::StaticData::Items[entry.randoItemId].spoilerName;
            spoiler["checks"][Rando::StaticData::Checks[entry.randoCheckId].name]["randoAct"] = entry.randoAct;
        } else {
            spoiler["checks"][Rando::StaticData::Checks[entry.randoCheckId].name] =
                Rando::StaticData::Items[entry.randoItemId].spoilerName;
        }
    }

    if (!shuffledEntrances.empty()) {
        spoiler["entrances"] = nlohmann::json::object();
        for (auto& entrance : shuffledEntrances) {
            spoiler["entrances"][Rando::StaticData::Entrances[entrance.randoEntranceId].name] =
                Rando::StaticData::Entrances[Rando::StaticData::GetEntranceIdFromDestination(entrance.destinationId)]
                    .name;
        }
    }

    return spoiler;
}

void GenerateFromSpoiler(nlohmann::json spoiler) {
    Rando::Logic::shuffledPool.clear();
    Rando::Logic::shuffledEntrances.clear();

    if (!spoiler.contains("type") || spoiler["type"] != "GHOSTSHIP_RANDO_SPOILER") {
        Notification::Emit({ .message = "Error: Invalid Spoiler Log.", .messageColor = ImVec4(0.85f, 0.3f, 0, 1) });
    }

    if (spoiler.contains("checks") && !spoiler["checks"].empty()) {
        for (auto& data : spoiler["checks"].items()) {
            struct LevelShuffleEntry checkEntry;
            for (auto& [checkId, staticCheck] : Rando::StaticData::Checks) {
                if (staticCheck.name == data.key()) {
                    checkEntry.randoCheckId = checkId;
                    break;
                }
            }

            std::string randoItemStr = "";
            if (data.value().contains("randoAct")) {
                checkEntry.randoAct = data.value()["randoAct"];
                randoItemStr = data.value()["randoItemId"];
            } else {
                checkEntry.randoAct = RA_ACT_NONE;
                randoItemStr = data.value();
            }

            for (auto& [itemId, staticItem] : Rando::StaticData::Items) {
                if (staticItem.spoilerName == randoItemStr) {
                    checkEntry.randoItemId = itemId;
                }
            }
            checkEntry.obtained = false;
            checkEntry.skipped = false;
            Rando::Logic::shuffledPool.push_back(checkEntry);
        }
    }

    if (spoiler.contains("entrances") && !spoiler["entrances"].empty()) {
        for (auto& data : spoiler["entrances"].items()) {
            RandoSaveEntrance randoSaveEntrance;
            for (auto& [entranceId, staticEntrance] : Rando::StaticData::Entrances) {
                if (staticEntrance.name == data.key()) {
                    randoSaveEntrance.randoEntranceId = entranceId;
                    randoSaveEntrance.found = false;
                }
                if (staticEntrance.name == data.value()) {
                    randoSaveEntrance.destinationId = staticEntrance.destinationId;
                }
            }

            Rando::Logic::shuffledEntrances.push_back(randoSaveEntrance);
        }
    }

    if (spoiler.contains("options") && !spoiler["options"].empty()) {
        for (auto& data : spoiler["options"].items()) {
            for (auto& [optionId, staticOption] : Rando::StaticData::Options) {
                RandoSaveOption randoSaveOption;
                if (staticOption.name == data.key()) {
                    RANDO_SAVE_OPTIONS(selectedFileNum)[optionId] = data.value().get<int32_t>();
                    break;
                }
            }
        }
    }
}

} // namespace Spoiler

} // namespace Rando
