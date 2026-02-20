#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_WDW] = RandoRegion{ .regionName = "Wet Dry World", .levelId = LEVEL_WDW,
        .checks = {
            CHECK(RC_WDW_BLUE_COIN_01, true),
            CHECK(RC_WDW_BLUE_COIN_02, true),
            CHECK(RC_WDW_BLUE_COIN_03, true),
            CHECK(RC_WDW_BLUE_COIN_04, true),
            CHECK(RC_WDW_BLUE_COIN_05, true),
            CHECK(RC_WDW_BLUE_COIN_06, true),
            CHECK(RC_WDW_RED_COIN_01, true),
            CHECK(RC_WDW_RED_COIN_02, true),
            CHECK(RC_WDW_RED_COIN_03, true),
            CHECK(RC_WDW_RED_COIN_04, true),
            CHECK(RC_WDW_RED_COIN_05, true),
            CHECK(RC_WDW_RED_COIN_06, true),
            CHECK(RC_WDW_RED_COIN_07, true),
            CHECK(RC_WDW_RED_COIN_08, true),
            CHECK(RC_WDW_STAR_01_ARROW_LIFTS, true),
            CHECK(RC_WDW_STAR_02_TOWN_TOP, true),
            CHECK(RC_WDW_STAR_03_SECRETS, true),
            CHECK(RC_WDW_STAR_04_ELEVATOR, true),
            CHECK(RC_WDW_STAR_05_RED_COINS, true),
            CHECK(RC_WDW_STAR_06_DOWNTOWN, true),
            CHECK(RC_WDW_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on