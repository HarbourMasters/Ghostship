#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_WF] = RandoRegion{ .regionName = "Whomp's Fortress", .levelId = LEVEL_WF,
        .checks = {
            CHECK(RC_WF_BLUE_COIN_01,           true),
            CHECK(RC_WF_BLUE_COIN_02,           true),
            CHECK(RC_WF_BLUE_COIN_03,           true),
            CHECK(RC_WF_BLUE_COIN_04,           true),
            CHECK(RC_WF_RED_COIN_01,            true),
            CHECK(RC_WF_RED_COIN_02,            HAS_COURSE_STAR(COURSE_WF, RA_ACT_01)),
            CHECK(RC_WF_RED_COIN_03,            true),
            CHECK(RC_WF_RED_COIN_04,            true),
            CHECK(RC_WF_RED_COIN_05,            true),
            CHECK(RC_WF_RED_COIN_06,            HAS_COURSE_STAR(COURSE_WF, RA_ACT_01)),
            CHECK(RC_WF_RED_COIN_07,            true),
            CHECK(RC_WF_RED_COIN_08,            true),
            CHECK(RC_WF_STAR_01_WHOMPS_BLOCK,   true),
            CHECK(RC_WF_STAR_02_FORTRESS_TOP,   HAS_COURSE_STAR(COURSE_WF, RA_ACT_01)),
            CHECK(RC_WF_STAR_03_WILD_BLUE,      CAN_ACCESS_ACT(COURSE_WF, RA_ACT_03)),
            CHECK(RC_WF_STAR_04_RED_COINS,      HAS_COURSE_STAR(COURSE_WF, RA_ACT_01)),
            CHECK(RC_WF_STAR_05_CAGED_ISLAND,   HAS_COURSE_STAR(COURSE_WF, RA_ACT_01)),
            CHECK(RC_WF_STAR_06_WALL,           CAN_ACCESS_ACT(COURSE_WF, RA_ACT_03)),
            CHECK(RC_WF_STAR_07_100_COIN,       CAN_ACCESS_ACT(COURSE_WF, RA_ACT_03)),
        },
    };
}, {});
// clang-format on