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

extern void RefreshChecksInLogic();

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

// Regions
struct RandoRegion {
    const char* regionName;
    int16_t levelId;
    std::map<RandoCheckId, std::pair<std::function<bool()>, std::string>> checks;
};

extern std::map<RandoRegionId, RandoRegion> Regions;

#define CHECK(check, condition)                               \
    {                                                         \
        check, {                                              \
            [] { return condition; }, LogicString(#condition) \
        }                                                     \
    }

// Logic Operators
#define HAS_COURSE_STAR(course, act) (CheckCourseActStar(course, act))
#define HAS_TARGET_STARS(count) (CheckTotalStars() >= count)
#define CAN_USE(item) (CheckFlagUnlock(RF_##item))

inline bool CheckCourseActStar(int16_t courseNum, int16_t starAct) {
    return courseNum == COURSE_NONE ? gSaveBuffer.files[selectedFileNum][0].flags & (1 << starAct)
                                    : gSaveBuffer.files[selectedFileNum][0].courseStars[courseNum] & (1 << starAct);
}

inline int32_t CheckTotalStars() {
    return save_file_get_total_star_count(selectedFileNum, COURSE_MIN - 1, COURSE_MAX - 1);
}

inline bool CheckFlagUnlock(int16_t flagType) {
    return gSaveBuffer.files[selectedFileNum][0].flags & (1 << (flagType + 1));
}

inline std::string LogicString(std::string condition) {
    if (condition == "true")
        return "";

    return condition;
}

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