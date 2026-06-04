#pragma once

#include "port/Engine.h"
#include "game/save_file.h"
#include "port/Rando/Rando.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/mods/achievements/Achievements.h"

#include <nlohmann/json.hpp>
#include <cstring>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShipSaveFeatures, achievements, rando)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveCheck, randoItemId, randoAct, obtained, skipped)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveEntrance, randoEntranceId, destinationId, found)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandoSaveData, randoSaveChecks, randoSaveEntrances, randoSaveOptions, finalSeed)

using json = nlohmann::json;

#define SAVE_FILE_VERSION 2
#define SAVE_FILE_VERSION_LEGACY 1
#define SAVE_FILE_VERSION_ACHIEVEMENT_MIGRATION 2

static std::string entries[] = {
    "BOB", "WF", "JRB", "CCM", "BBH", "HMC", "LLL", "SSL", "DDD", "SL",
    "WDW", "TTM", "THI", "TTC", "RR", "BITDW", "BITFS", "BITS", "PSS", "COTMC",
    "TOTWC", "VCUTM", "WMOTR", "SA", "CG", "CAKE_END"
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

inline void ReconstructAchievementData(const SaveFile& save, AchievementSaveData& achievementData) {
    achievementData.cheated = false;
    achievementData.capStars = 0;

    int32_t maxCoins = 0;
    for (size_t i = 0; i < COURSE_STAGES_COUNT; i++) {
        if (save.courseCoinScores[i] > maxCoins) {
            maxCoins = save.courseCoinScores[i];
        }
    }
    achievementData.coins = maxCoins;
}

inline void from_json(const json& j, AchievementSaveEntry& entry) {
    const auto id = GetSafeEntry<std::string>(j, "id");
    const auto progress = GetSafeEntry<int32_t>(j, "progress");

    entry.id = (char*) GameEngine_Malloc(id.length() + 1);
    std::strcpy(const_cast<char *>(entry.id), id.c_str());

    entry.progress = progress;
}

inline void to_json(json& j, const AchievementSaveEntry& entry) {
    j = json{
        { "id", std::string(entry.id) },
        { "progress", entry.progress }
    };
}

inline void from_json(const json& j, AchievementSaveData& data) {
    data.cheated = j.at("cheated").get<bool>();
    data.capStars = GetSafeEntry(j, "capStars", 0);
    data.coins = GetSafeEntry(j, "coins", 0);

    if (j.contains("entries")) {
        auto entriesJson = j.at("entries");
        for (size_t i = 0; i < entriesJson.size(); i++) {
            data.entries[i] = entriesJson.at(i).get<AchievementSaveEntry>();
        }
    }
}

inline void to_json(json& j, const AchievementSaveData& data) {
    json entriesJson = json::array();
    for (size_t i = 0; i < gAchievementList.size(); i++) {
        entriesJson.push_back(data.entries[i]);
    }

    j = json{
        { "cheated", data.cheated },
        { "capStars",data.capStars },
        { "coins", data.coins },
        { "entries", entriesJson }
    };
}

inline void from_json(const json& j, ShipSaveData& save) {
    save.features = j.at("features").get<ShipSaveFeatures>();

    if (save.features.rando) {
        j["randoSaveData"].get_to(save.randoSaveData);
    }

    if (j.contains("achievementSaveData")) {
        j["achievementSaveData"].get_to(save.achievementSaveData);
    } else if (save.features.achievements) {
        memset(&save.achievementSaveData, 0, sizeof(AchievementSaveData));
        save.achievementSaveData.cheated = false;
        save.achievementSaveData.capStars = 0;
        save.achievementSaveData.coins = 0;
    }
}

inline void to_json(json& j, const ShipSaveData& save) {
    j = json{ { "features", save.features } };

    if (save.features.rando) {
        j["randoSaveData"] = save.randoSaveData;
    }

    if (save.features.achievements) {
        j["achievementSaveData"] = save.achievementSaveData;
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

inline void LoadSaveFileLegacy(const json& j, SaveFile& save) {
    json capPosJson = GetSafeEntry<json>(j, "capPos", json::object());
    save.capLevel = GetSafeEntry(j, "capLevel", 0);
    save.capArea = GetSafeEntry(j, "capArea", 0);
    save.capPos[0] = GetSafeEntry(capPosJson, "x", 0);
    save.capPos[1] = GetSafeEntry(capPosJson, "y", 0);
    save.capPos[2] = GetSafeEntry(capPosJson, "z", 0);
    save.flags = GetSafeEntry(j, "flags", 0);

    json starsJson = GetSafeEntry<json>(j, "courseStars", json::object());
    for (size_t i = 0; i < COURSE_COUNT; i++) {
        save.courseStars[i] = GetSafeEntry<u8>(starsJson, entries[i], static_cast<u8>(0));
    }

    json coinsJson = GetSafeEntry<json>(j, "courseCoinScores", json::object());
    for (size_t i = 0; i < COURSE_STAGES_COUNT; i++) {
        save.courseCoinScores[i] = GetSafeEntry<u8>(coinsJson, entries[i], static_cast<u8>(0));
    }

    memset(&save.shipSaveData, 0, sizeof(ShipSaveData));
    if (j.contains("shipSaveData")) {
        j["shipSaveData"].get_to(save.shipSaveData);

        // Migrate old saves that don't have achievement data
        bool hasMissingAchievementData = save.shipSaveData.features.achievements &&
                                         !j["shipSaveData"].contains("achievementSaveData");
        if (hasMissingAchievementData) {
            ReconstructAchievementData(save, save.shipSaveData.achievementSaveData);
        }
    }
}

inline void LoadSaveFileV2(const json& j, SaveFile& save) {
    json capPosJson = GetSafeEntry<json>(j, "capPos", json::object());
    save.capLevel = GetSafeEntry(j, "capLevel", 0);
    save.capArea = GetSafeEntry(j, "capArea", 0);
    save.capPos[0] = GetSafeEntry(capPosJson, "x", 0);
    save.capPos[1] = GetSafeEntry(capPosJson, "y", 0);
    save.capPos[2] = GetSafeEntry(capPosJson, "z", 0);
    save.flags = GetSafeEntry(j, "flags", 0);

    json starsJson = GetSafeEntry<json>(j, "courseStars", json::object());
    for (size_t i = 0; i < COURSE_COUNT; i++) {
        save.courseStars[i] = GetSafeEntry<u8>(starsJson, entries[i], static_cast<u8>(0));
    }

    json coinsJson = GetSafeEntry<json>(j, "courseCoinScores", json::object());
    for (size_t i = 0; i < COURSE_STAGES_COUNT; i++) {
        save.courseCoinScores[i] = GetSafeEntry<u8>(coinsJson, entries[i], static_cast<u8>(0));
    }

    memset(&save.shipSaveData, 0, sizeof(ShipSaveData));
    if (j.contains("shipSaveData")) {
        j["shipSaveData"].get_to(save.shipSaveData);
    }
}

inline void from_json(const json& j, SaveFile& save) {
    uint32_t version = GetSafeEntry<uint32_t>(j, "version", 1u);
    if (version == 1) {
        LoadSaveFileLegacy(j, save);
    } else if (version == 2) {
        LoadSaveFileV2(j, save);
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