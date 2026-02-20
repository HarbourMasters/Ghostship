#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_SL] = RandoRegion{ .regionName = "Snowman's Land", .levelId = LEVEL_SL,
        .checks = {
            CHECK(RC_SL_RED_COIN_01, true),
            CHECK(RC_SL_RED_COIN_02, true),
            CHECK(RC_SL_RED_COIN_03, true),
            CHECK(RC_SL_RED_COIN_04, true),
            CHECK(RC_SL_RED_COIN_05, true),
            CHECK(RC_SL_RED_COIN_06, true),
            CHECK(RC_SL_RED_COIN_07, true),
            CHECK(RC_SL_RED_COIN_08, true),
            CHECK(RC_SL_STAR_01_BIG_HEAD, true),
            CHECK(RC_SL_STAR_02_BULLY, true),
            CHECK(RC_SL_STAR_03_DEEP_FREEZE, true),
            CHECK(RC_SL_STAR_04_FREEZING_POND, true),
            CHECK(RC_SL_STAR_05_RED_COINS, true),
            CHECK(RC_SL_STAR_06_IGLOO, true),
            CHECK(RC_SL_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on