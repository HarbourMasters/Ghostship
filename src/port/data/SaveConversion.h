#pragma once

#include "game/save_file.h"
#include "port/Rando/Rando.h"
#include "port/Rando/Spoiler/Spoiler.h"

#include <nlohmann/json.hpp>
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveCheck, randoItemId, randoAct, obtained, skipped)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveEntrance, randoEntranceId, destinationId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveData, randoSaveChecks, randoSaveEntrances, randoSaveOptions, finalSeed)

using json = nlohmann::json;

#define SAVE_FILE_VERSION 1

static std::string entries[] = {
    "CG", "BOB", "WF", "JRB", "CCM", "BBH", "HMC", "LLL", "SSL", "DDD", "SL",
    "WDW", "TTM", "THI", "TTC", "RR", "BITDW", "BITFS", "BITS", "PSS", "COTMC",
    "TOTWC", "VCUTM", "WMOTR", "SA", "CAKE_END"
};

template<typename T>
T GetSafeEntry(const json& node, const std::string& key) {
    if(!node.contains(key)) {
        auto dump = nlohmann::json(node).dump(4);
        throw std::runtime_error("JSON missing the '" + key + "' entry\nProblematic JSON:\n" + dump);
    }

    return node.at(key).get<T>();
}

template<typename T>
T GetSafeEntry(const json& node, const std::string& key, const T& def) {
    if(!node.contains(key)) {
        return def;
    }

    return node.at(key).get<T>();
}

inline void from_json(const json& j, ShipSaveData& save) {
    save.saveType = j.value("saveType", SAVETYPE_VANILLA);

    if (save.saveType == SAVETYPE_RANDO) {
        j["randoSaveData"].get_to(save.randoSaveData);
    }
}

inline void to_json(json& j, const ShipSaveData& save) {
    j = json{ { "saveType", save.saveType } };

    if (save.saveType == SAVETYPE_RANDO) {
        j["randoSaveData"] = save.randoSaveData;
    }
}

inline void to_json(json& j, const SaveFile& save) {
    json stars = {};
    json coins = {};
    json cap = {
        { "x", save.capPos[0] },
        { "y", save.capPos[1] },
        { "z", save.capPos[2] },
    };

    for (size_t i = 0; i < COURSE_COUNT; i++) {
        stars[entries[i]] = save.courseStars[i];
    }

    for (size_t i = 0; i < COURSE_STAGES_COUNT; i++) {
        coins[entries[i]] = save.courseCoinScores[i];
    }

    j = json{
        { "version", SAVE_FILE_VERSION },
        { "capLevel", save.capLevel },
        { "capArea", save.capArea },
        { "capPos", cap },
        { "flags", save.flags },
        { "courseStars", stars },
        { "courseCoinScores", coins },
        { "shipSaveData", save.shipSaveData }
    };
}

inline void LoadSaveFileV1(const json& j, SaveFile& save) {
    json capPosJson = GetSafeEntry<json>(j, "capPos");
    save.capLevel = GetSafeEntry(j, "capLevel", 0);
    save.capArea = GetSafeEntry(j, "capArea", 0);
    save.capPos[0] = GetSafeEntry(capPosJson, "x", 0);
    save.capPos[1] = GetSafeEntry(capPosJson, "y", 0);
    save.capPos[2] = GetSafeEntry(capPosJson, "z", 0);
    save.flags = GetSafeEntry(j, "flags", 0);

    json starsJson = GetSafeEntry<json>(j, "courseStars");
    for (size_t i = 0; i < COURSE_COUNT; i++) {
        save.courseStars[i] = GetSafeEntry<u8>(starsJson, entries[i]);
    }

    json coinsJson = GetSafeEntry<json>(j, "courseCoinScores");
    for (size_t i = 0; i < COURSE_STAGES_COUNT; i++) {
        save.courseCoinScores[i] = GetSafeEntry<u8>(coinsJson, entries[i]);
    }

    if (j.contains("shipSaveData")) {
        j["shipSaveData"].get_to(save.shipSaveData);
    }
}

inline void from_json(const json& j, SaveFile& save) {
    uint32_t version = GetSafeEntry<uint32_t>(j, "version");
    if(version == 1) {
        LoadSaveFileV1(j, save);
    }
}

inline void to_json(json& j, const MainMenuSaveData& menu) {
    json coins = json::array();
    for (size_t i = 0; i < NUM_SAVE_FILES; i++) {
        coins.push_back(menu.coinScoreAges[i]);
    }

    j = json{
        { "version", SAVE_FILE_VERSION },
        { "coinScoreAges", coins },
        { "soundMode", menu.soundMode }
    };
}

inline void LoadMainMenuSaveDataV1(const json& j, MainMenuSaveData& menu) {
    json coinsJson = GetSafeEntry<json>(j, "coinScoreAges");
    for (size_t i = 0; i < NUM_SAVE_FILES; i++) {
        menu.coinScoreAges[i] = coinsJson.at(i).get<u32>();
    }

    menu.soundMode = GetSafeEntry<u16>(j, "soundMode", 0);
}

inline void from_json(const json& j, MainMenuSaveData& menu) {
    uint32_t version = GetSafeEntry<uint32_t>(j, "version");
    if(version == 1) {
        LoadMainMenuSaveDataV1(j, menu);
    }
}