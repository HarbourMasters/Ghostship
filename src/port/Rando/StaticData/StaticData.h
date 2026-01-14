#ifndef RANDO_STATIC_DATA_H
#define RANDO_STATIC_DATA_H

#include <map>
#include <array>
#include "port/Rando/Types.h"

#include "include/level_table.h"
#include "include/model_ids.h"
#include "include/types.h"

namespace Rando {

namespace StaticData {

struct RandoStaticCheck {
    RandoCheckId randoCheckId;
    const char* name;
    RandoCheckType randoCheckType;
    LevelNum levelId;
    RandoItemId randoItemId;
    int16_t posX;
    int16_t posY;
    int16_t posZ;
};

extern std::map<RandoCheckId, RandoStaticCheck> Checks;

RandoCheckId GetCheckIdFromName(const char* name);

struct RandoStaticItem {
    RandoItemId randoItemId;
    const char* spoilerName;
    const char* article;
    const char* name;
    RandoItemType randoItemType;
    int16_t modelId;
};

extern std::map<RandoItemId, RandoStaticItem> Items;
extern std::map<RandoCheckId, RandoItemId> shuffledList;

RandoCheckId GetCheckByLocation(int16_t posX, int16_t posY, int16_t posZ);
int16_t GetModelByRandoItem(RandoItemId randoItem);
const BehaviorScript *GetBehaviorByModel(int16_t modelId);
RandoItemId GetShuffledRandoItem(RandoCheckId randoCheckId);
void ShuffleItemList();

//  TODO: Add RandoStaticOptions
// struct RandoStaticOption {
//     RandoOptionId randoOptionId;
//     const char* name;
//     const char* cvar;
//     s32 defaultValue;
// };

// extern std::map<RandoOptionId, RandoStaticOption> Options;

// RandoOptionId GetOptionIdFromName(const char* name);

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
