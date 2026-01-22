#ifndef RANDO_STATIC_DATA_H
#define RANDO_STATIC_DATA_H

#include <map>
#include <array>
#include "port/Rando/Types.h"
#include "port/Rando/Logic/Logic.h"

#include "include/level_table.h"
#include "include/model_ids.h"
#include "include/types.h"

namespace Rando {

namespace StaticData {

struct RandoCustomData {
    RandoCheckId randoCheckId;
    RandoItemId randoItemId;
    RandoAct randoActNum;
    bool isShuffled;
};

struct RandoStaticCheck {
    RandoCheckId randoCheckId;
    const char* name;
    RandoCheckType randoCheckType;
    LevelNum levelId;
    RandoAct actData;
    RandoItemId randoItemId;
    int16_t posX;
    int16_t posY;
    int16_t posZ;
};

extern std::map<RandoCheckId, RandoStaticCheck> Checks;
extern RandoStaticCheck GetShuffledRandoStaticCheck(s16 x, s16 y, s16 z);

struct RandoStaticItem {
    RandoItemId randoItemId;
    const char* spoilerName;
    const char* article;
    const char* name;
    RandoItemType randoItemType;
    int16_t modelId;
};

RandoCheckId GetCheckByLocation(int16_t posX, int16_t posY, int16_t posZ);
bool IsCheckShuffled(int16_t levelId, RandoCheckId randocheckId);

extern std::map<RandoItemId, RandoStaticItem> Items;

int16_t GetModelByRandoItem(RandoItemId randoItem);
const BehaviorScript *GetBehaviorByModel(int16_t modelId);
int16_t GetModelByBehavior(const BehaviorScript* behavior);
RandoItemId GetShuffledRandoItem(int16_t currentCourse, RandoCheckId randoCheckId);
RandoAct GetShuffledRandoAct(int16_t currentCourse, RandoCheckId randoCheckId);

struct RandoStaticOption {
    RandoOptionId randoOptionId;
    const char* name;
    const char* cvar;
    s32 defaultValue;
};

extern std::map<RandoOptionId, RandoStaticOption> Options;
extern std::unordered_map<int32_t, const char*> logicOptions;

RandoOptionId GetOptionIdFromName(const char* name);

// TODO: Import Object Extension
extern std::map<RandoCheckId, struct Object*> spawnedRandoObjects;

// TODO: Add Logic and Regions
// struct RandoStaticRegion {
//     RandoRegionId randoRegionId;
//     const char* name;
//     LevelNum levelId;
//     std::map<RandoCheckId, std::function<bool()>> checks;
//     std::map<RandoRegionId, std::function<bool()>> regions;
// };

// extern std::map<RandoRegionId, RandoStaticRegion> Regions;

} // namespace StaticData

} // namespace Rando

#endif // RANDO_STATIC_DATA_H
