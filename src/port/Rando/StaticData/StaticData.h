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
// void PopulateCheckNames();

struct RandoStaticItem {
    RandoItemId randoItemId;
    const char* spoilerName;
    const char* article;
    const char* name;
    RandoItemType randoItemType;
    int16_t modelId;
};

extern std::map<RandoItemId, RandoStaticItem> Items;

RandoCheckId GetCheckByLocation(int16_t posX, int16_t posY, int16_t posZ);
int16_t GetModelByRandoItem(RandoItemId randoItem);
const BehaviorScript *GetBehaviorByModel(int16_t modelId);
// extern std::unordered_map<StartingItemCategory, std::vector<RandoItemId>> StartingItemsMap;

// RandoItemId GetItemIdFromName(const char* name);
// u8 GetIconForZMessage(RandoItemId itemId);
// const char* GetIconTexturePath(RandoItemId itemId);
// bool ShouldShowGetItemCutscene(RandoItemId itemId);
// std::string GetItemName(RandoItemId randoItemId, bool includeArticle = true);
// std::string GetTrapMessage();

// struct RandoStaticOption {
//     RandoOptionId randoOptionId;
//     const char* name;
//     const char* cvar;
//     s32 defaultValue;
// };

// extern std::map<RandoOptionId, RandoStaticOption> Options;

// RandoOptionId GetOptionIdFromName(const char* name);

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
