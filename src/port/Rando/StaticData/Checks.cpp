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
    RC(RC_BBH_RED_COIN_01,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    -1164, 0, 2325),
    RC(RC_BBH_RED_COIN_02,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    2540, 820, 2174),
    RC(RC_BBH_RED_COIN_03,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    2600, 815, -380),
    RC(RC_BBH_RED_COIN_04,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    2950, 815, -1250),
    RC(RC_BBH_RED_COIN_05,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    -1500, 550, 960),
    RC(RC_BBH_RED_COIN_06,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    -420, 550, 60),
    RC(RC_BBH_RED_COIN_07,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    2856, 50, 2144),
    RC(RC_BBH_RED_COIN_08,      RCTYPE_MAJOR,   LEVEL_BBH,          RI_COIN_RED,    50, 900, -1400),
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
    RC(RC_CCM_RED_COIN_01,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    -488, 3100, -2305),
    RC(RC_CCM_RED_COIN_02,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    -3120, -1200, -2080),
    RC(RC_CCM_RED_COIN_03,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    2940, -1400, 2630),
    RC(RC_CCM_RED_COIN_04,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    -600, -505, 1940),
    RC(RC_CCM_RED_COIN_05,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    -3814, -2112, 6427),
    RC(RC_CCM_RED_COIN_06,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    2700, -4600, 1230),
    RC(RC_CCM_RED_COIN_07,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    1990, -4040, 4950),
    RC(RC_CCM_RED_COIN_08,      RCTYPE_MAJOR,   LEVEL_CCM,          RI_COIN_RED,    4408, -4550, -1534),
    RC(RC_COTMC_RED_COIN_01,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    200, -291, -5600),
    RC(RC_COTMC_RED_COIN_02,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    980, 260, -3430),
    RC(RC_COTMC_RED_COIN_03,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    -540, -352, -5940),
    RC(RC_COTMC_RED_COIN_04,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    -300, 450, -6240),
    RC(RC_COTMC_RED_COIN_05,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    -200, -400, -6680),
    RC(RC_COTMC_RED_COIN_06,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    250, 450, -6400),
    RC(RC_COTMC_RED_COIN_07,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    540, -361, -6340),
    RC(RC_COTMC_RED_COIN_08,    RCTYPE_MAJOR,   LEVEL_COTMC,        RI_COIN_RED,    980, 260, -3810),
    RC(RC_SL_RED_COIN_01,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    4750, 1061, -5230),
    RC(RC_SL_RED_COIN_02,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    416, 1050, -4522),
    RC(RC_SL_RED_COIN_03,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -6560, 2040, -5080),
    RC(RC_SL_RED_COIN_04,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -6760, 2040, -1360),
    RC(RC_SL_RED_COIN_05,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -6880, 1857, 1000),
    RC(RC_SL_RED_COIN_06,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -4211, 1092, -4723),
    RC(RC_SL_RED_COIN_07,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -6271, 1390, 4764),
    RC(RC_SL_RED_COIN_08,       RCTYPE_MAJOR,   LEVEL_SL,           RI_COIN_RED,    -529, 1050, -5329),
    RC(RC_SSL_RED_COIN_01,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    -4500, 1500, -3500),
    RC(RC_SSL_RED_COIN_02,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    -4500, 1500, 1500),
    RC(RC_SSL_RED_COIN_03,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    500, 1500, -3500),
    RC(RC_SSL_RED_COIN_04,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    500, 1500, 1500),
    RC(RC_SSL_RED_COIN_05,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    3380, 10, -4900),
    RC(RC_SSL_RED_COIN_06,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    -5270, -170, -6780),
    RC(RC_SSL_RED_COIN_07,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    -7450, 10, 7560),
    RC(RC_SSL_RED_COIN_08,      RCTYPE_MAJOR,   LEVEL_SSL,          RI_COIN_RED,    5900, 70, 2311),
    RC(RC_WF_RED_COIN_01,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    -250, 2650, 2970),
    RC(RC_WF_RED_COIN_02,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    1746, 3620, -3120),
    RC(RC_WF_RED_COIN_03,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    1277, 2600, 1350),
    RC(RC_WF_RED_COIN_04,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    1585, 2595, -80),
    RC(RC_WF_RED_COIN_05,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    3350, 3000, -1520),
    RC(RC_WF_RED_COIN_06,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    2700, 3600, -900),
    RC(RC_WF_RED_COIN_07,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    3770, 1380, 650),
    RC(RC_WF_RED_COIN_08,       RCTYPE_MAJOR,   LEVEL_WF,           RI_COIN_RED,    -270, 1720, 2250),
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
