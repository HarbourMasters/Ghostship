#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_RR] = RandoRegion{ .regionName = "Rainbow Ride", .levelId = LEVEL_RR,
        .checks = {
            CHECK(RC_RR_BLUE_COIN_01, true),
            CHECK(RC_RR_BLUE_COIN_02, true),
            CHECK(RC_RR_BLUE_COIN_03, true),
            CHECK(RC_RR_BLUE_COIN_04, true),
            CHECK(RC_RR_BLUE_COIN_05, true),
            CHECK(RC_RR_BLUE_COIN_06, true),
            CHECK(RC_RR_RED_COIN_01, true),
            CHECK(RC_RR_RED_COIN_02, true),
            CHECK(RC_RR_RED_COIN_03, true),
            CHECK(RC_RR_RED_COIN_04, true),
            CHECK(RC_RR_RED_COIN_05, true),
            CHECK(RC_RR_RED_COIN_06, true),
            CHECK(RC_RR_RED_COIN_07, true),
            CHECK(RC_RR_RED_COIN_08, true),
            CHECK(RC_RR_STAR_01_CRUISER, true),
            CHECK(RC_RR_STAR_02_BIG_HOUSE, true),
            CHECK(RC_RR_STAR_03_RED_COINS, true),
            CHECK(RC_RR_STAR_04_SWINGIN_BREEZE, true),
            CHECK(RC_RR_STAR_05_TRIANGLES, true),
            CHECK(RC_RR_STAR_06_OVER_THE_RAINBOW, true),
            CHECK(RC_RR_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on