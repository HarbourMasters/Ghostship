#include "StaticData.h"
#include "port/ShipUtils.h"

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

extern "C" {
#include "game/area.h"
}

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
    RC(RC_UNKNOWN,              RCTYPE_UNKNOWN, LEVEL_UNKNOWN_1,    RI_UNKNOWN,     0, 0, 0),
    RC(RC_BOB_STAR_CHAIN_CHOMP, RCTYPE_MAJOR,   LEVEL_BOB,          RI_STAR,        1550, 1200, 300),
    RC(RC_BOB_STAR_KING_BOB,    RCTYPE_MAJOR,   LEVEL_BOB,          RI_STAR,        2000, 4500, -4500),
    RC(RC_BOB_STAR_RED_COIN,    RCTYPE_MAJOR,   LEVEL_BOB,          RI_STAR,        -6000, 1000, 2400),
    RC(RC_BOB_RED_COIN_01,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    -5500, 768, 2400),
    RC(RC_BOB_RED_COIN_02,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    -6500, 768, 2400),
    RC(RC_BOB_RED_COIN_03,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    1135, 1920, -7161),
    RC(RC_BOB_RED_COIN_04,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    250, 1000, 1920),
    RC(RC_BOB_RED_COIN_05,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    1600, 980, 4440),
    RC(RC_BOB_RED_COIN_06,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    -2755, 0, -4083),
    RC(RC_BOB_RED_COIN_07,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    4934, 1352, 861),
    RC(RC_BOB_RED_COIN_08,      RCTYPE_MAJOR,   LEVEL_BOB,          RI_COIN_RED,    4100, 3672, 1629),
};
// clang-format on

RandoCheckId GetCheckByLocation(int16_t posX, int16_t posY, int16_t posZ) {
    for (auto& [randoCheckId, randoStaticCheck] : Rando::StaticData::Checks) {
        if (gCurrLevelNum != randoStaticCheck.levelId) {
            continue;
        }
        if (posX == randoStaticCheck.posX && posY == randoStaticCheck.posY && posZ == randoStaticCheck.posZ) {
            return randoCheckId;
        }
    }
    return RC_UNKNOWN;
}

} // namespace StaticData
} // namespace Rando
