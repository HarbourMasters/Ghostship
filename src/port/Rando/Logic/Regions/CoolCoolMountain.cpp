#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_CCM] = RandoRegion{ .regionName = "Cool Cool Mountain", .levelId = LEVEL_CCM,
        .checks = {
            CHECK(RC_CCM_BLUE_COIN_01, true),
            CHECK(RC_CCM_BLUE_COIN_02, true),
            CHECK(RC_CCM_BLUE_COIN_03, true),
            CHECK(RC_CCM_RED_COIN_01, true),
            CHECK(RC_CCM_RED_COIN_02, true),
            CHECK(RC_CCM_RED_COIN_03, true),
            CHECK(RC_CCM_RED_COIN_04, true),
            CHECK(RC_CCM_RED_COIN_05, true),
            CHECK(RC_CCM_RED_COIN_06, true),
            CHECK(RC_CCM_RED_COIN_07, true),
            CHECK(RC_CCM_RED_COIN_08, true),
            CHECK(RC_CCM_STAR_01_SLIDING_AWAY, true),
            CHECK(RC_CCM_STAR_02_LOST_PENGUIN, true),
            CHECK(RC_CCM_STAR_03_PENGUIN_RACE, HAS_COURSE_STAR(COURSE_CCM, RA_ACT_02)),
            CHECK(RC_CCM_STAR_04_RED_COINS, true),
            CHECK(RC_CCM_STAR_05_LOST_HEAD, HAS_COURSE_STAR(COURSE_CCM, RA_ACT_04)),
            CHECK(RC_CCM_STAR_06_WALL_KICKS, true),
            CHECK(RC_CCM_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on