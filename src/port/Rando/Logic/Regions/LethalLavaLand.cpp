#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_LLL] = RandoRegion{ .regionName = "Lethal Lava Land", .levelId = LEVEL_LLL,
        .checks = {
            CHECK(RC_LLL_RED_COIN_01,               true),
            CHECK(RC_LLL_RED_COIN_02,               true),
            CHECK(RC_LLL_RED_COIN_03,               true),
            CHECK(RC_LLL_RED_COIN_04,               true),
            CHECK(RC_LLL_RED_COIN_05,               true),
            CHECK(RC_LLL_RED_COIN_06,               true),
            CHECK(RC_LLL_RED_COIN_07,               true),
            CHECK(RC_LLL_RED_COIN_08,               true),
            CHECK(RC_LLL_STAR_01_BIG_BULLY,         true),
            CHECK(RC_LLL_STAR_02_LITTLE_BULLIES,    true),
            CHECK(RC_LLL_STAR_03_RED_COINS,         true),
            CHECK(RC_LLL_STAR_04_LOG_ROLLING,       true),
            CHECK(RC_LLL_STAR_05_VOLCANO_FOOT,      true),
            CHECK(RC_LLL_STAR_06_VOLCANO_ELEVATOR,  true),
            CHECK(RC_LLL_STAR_07_100_COIN,          true),
        },
    };
}, {});
// clang-format on