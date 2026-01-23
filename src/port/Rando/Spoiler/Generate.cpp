#include "Spoiler.h"
#include "port/Rando/Logic/Logic.h"
#include "port/Rando/Rando.h"
#include "port/ui/Notification.h"

namespace Rando {

namespace Spoiler {

nlohmann::json GenerateFromPoolGeneration(std::vector<LevelShuffleEntry>& shuffledPool) {
    nlohmann::json spoiler;
    spoiler["type"] = "GHOSTSHIP_RANDO_SPOILER";
    spoiler["fileNum"] = std::to_string(selectedFileNum);
    // spoiler["commitHash"] = gSaveContext.save.shipSaveInfo.commitHash;
    // spoiler["finalSeed"] = gSaveContext.save.shipSaveInfo.rando.finalSeed;

    // TODO: Add once JSON Saves are in.
    // spoiler["options"] = nlohmann::json::object();
    // for (auto& [randoOptionId, randoStaticOption] : Rando::StaticData::Options) {
    //     spoiler["options"][randoStaticOption.name] = RANDO_SAVE_OPTIONS[randoOptionId];
    // }

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

    return spoiler;
}
std::vector<LevelShuffleEntry> GenerateFromSpoilerLog(nlohmann::json spoiler) {
    std::vector<LevelShuffleEntry> spoilerChecks;

    if (!spoiler.contains("type") || spoiler["type"] != "GHOSTSHIP_RANDO_SPOILER") {
        Notification::Emit({ .message = "Error: Invalid Spoiler Log.", .messageColor = ImVec4(0.85f, 0.3f, 0, 1) });
        return spoilerChecks;
    }

    if (spoiler.contains("checks") && !spoiler["checks"].empty()) {
        for (auto& data : spoiler["checks"].items()) {
            struct LevelShuffleEntry levelEntry;
            for (auto& [checkId, staticCheck] : Rando::StaticData::Checks) {
                if (staticCheck.name == data.key()) {
                    levelEntry.randoCheckId = checkId;
                    break;
                }
            }

            std::string randoItemStr = "";
            if (data.value().contains("randoAct")) {
                levelEntry.randoAct = data.value()["randoAct"];
                randoItemStr = data.value()["randoItemId"];
            } else {
                levelEntry.randoAct = RA_ACT_NONE;
                randoItemStr = data.value();
            }

            for (auto& [itemId, staticItem] : Rando::StaticData::Items) {
                if (staticItem.spoilerName == randoItemStr) {
                    levelEntry.randoItemId = itemId;
                }
            }
            spoilerChecks.push_back(levelEntry);
        }
    }

    return spoilerChecks;
}

} // namespace Spoiler

} // namespace Rando
