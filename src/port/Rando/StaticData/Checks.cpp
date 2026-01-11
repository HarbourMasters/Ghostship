#include "StaticData.h"
#include "port/ShipUtils.h"

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

namespace Rando {

namespace StaticData {
std::array<std::string, RC_MAX> CheckNames = std::array<std::string, RC_MAX>();

#define RC(id, type, level, item, x, y, z)      \
    {                                           \
        id, {                                   \
            id, #id, type, level, item, x, y, z \
        }                                       \
    }

// clang-format off
std::map<RandoCheckId, RandoStaticCheck> Checks = {
    RC(RC_UNKNOWN,                          RCTYPE_UNKNOWN, LEVEL_UNKNOWN_1,        RI_UNKNOWN,     0, 0, 0),
    RC(RC_CASTLE_GROUNDS_YELLOW_COIN_01,    RCTYPE_MAJOR,   LEVEL_CASTLE_GROUNDS,   RI_COIN_YELLOW, 0, 540, -1774),
    RC(RC_CASTLE_GROUNDS_SIGNPOST_01,       RCTYPE_MAJOR,   LEVEL_CASTLE_GROUNDS,   RI_SIGNPOST,    5288, 722, -800),
};
// clang-format on

RandoCheckId GetCheckByLocation(int16_t posX, int16_t posY, int16_t posZ) {
    for (auto& [randoCheckId, randoStaticCheck] : Rando::StaticData::Checks) {
        if (posX == randoStaticCheck.posX && posY == randoStaticCheck.posY && posZ == randoStaticCheck.posZ) {
            return randoCheckId;
        }
    }
    return RC_UNKNOWN;
}

RandoCheckId GetCheckIdFromName(const char* name) {
    for (auto& [randoCheckId, randoStaticCheck] : Checks) {
        if (strcmp(name, randoStaticCheck.name) == 0) {
            return randoCheckId;
        }
    }
    return RC_UNKNOWN;
}

} // namespace StaticData
} // namespace Rando
