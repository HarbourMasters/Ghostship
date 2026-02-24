#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_DDD] = RandoRegion{ .regionName = "Dire Dire Docks", .levelId = LEVEL_DDD,
        .checks = {
            CHECK(RC_DDD_BLUE_COIN_01, true),
            CHECK(RC_DDD_BLUE_COIN_02, true),
            CHECK(RC_DDD_BLUE_COIN_03, true),
            CHECK(RC_DDD_BLUE_COIN_04, true),
            CHECK(RC_DDD_BLUE_COIN_05, true),
            CHECK(RC_DDD_BLUE_COIN_06, true),
            CHECK(RC_DDD_RED_COIN_01, true),
            CHECK(RC_DDD_RED_COIN_02, true),
            CHECK(RC_DDD_RED_COIN_03, true),
            CHECK(RC_DDD_RED_COIN_04, true),
            CHECK(RC_DDD_RED_COIN_05, true),
            CHECK(RC_DDD_RED_COIN_06, true),
            CHECK(RC_DDD_RED_COIN_07, true),
            CHECK(RC_DDD_RED_COIN_08, true),
            CHECK(RC_DDD_STAR_01_BOWSERS_SUB, true),
            CHECK(RC_DDD_STAR_02_CHESTS, true),
            CHECK(RC_DDD_STAR_03_RED_COINS, HAS_COURSE_STAR(COURSE_DDD, RA_ACT_01)),
            CHECK(RC_DDD_STAR_04_JET_STREAM, CAN_USE(METAL)),
            CHECK(RC_DDD_STAR_05_MANTA_RAY, HAS_COURSE_STAR(COURSE_DDD, RA_ACT_01)),
            CHECK(RC_DDD_STAR_06_CAPS, CAN_USE(VANISH)),
            CHECK(RC_DDD_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on