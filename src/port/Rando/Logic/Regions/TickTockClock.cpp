#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_TTC] = RandoRegion{ .regionName = "Tick Tock Clock", .levelId = LEVEL_TTC,
        .checks = {
            CHECK(RC_TTC_BLUE_COIN_01, true),
            CHECK(RC_TTC_BLUE_COIN_02, true),
            CHECK(RC_TTC_BLUE_COIN_03, true),
            CHECK(RC_TTC_BLUE_COIN_04, true),
            CHECK(RC_TTC_BLUE_COIN_05, true),
            CHECK(RC_TTC_BLUE_COIN_06, true),
            CHECK(RC_TTC_BLUE_COIN_07, true),
            CHECK(RC_TTC_RED_COIN_01, true),
            CHECK(RC_TTC_RED_COIN_02, true),
            CHECK(RC_TTC_RED_COIN_03, true),
            CHECK(RC_TTC_RED_COIN_04, true),
            CHECK(RC_TTC_RED_COIN_05, true),
            CHECK(RC_TTC_RED_COIN_06, true),
            CHECK(RC_TTC_RED_COIN_07, true),
            CHECK(RC_TTC_RED_COIN_08, true),
            CHECK(RC_TTC_STAR_01_CAGE, true),
            CHECK(RC_TTC_STAR_02_PENDULUMS, true),
            CHECK(RC_TTC_STAR_03_HAND, true),
            CHECK(RC_TTC_STAR_04_THWOMP, true),
            CHECK(RC_TTC_STAR_05_MOVING_BARS, true),
            CHECK(RC_TTC_STAR_06_RED_COINS, true),
            CHECK(RC_TTC_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on