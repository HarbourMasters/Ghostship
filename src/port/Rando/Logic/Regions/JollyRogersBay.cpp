#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_JRB] = RandoRegion{ .regionName = "Jolly Roger's Bay", .levelId = LEVEL_JRB,
        .checks = {
            CHECK(RC_JRB_BLUE_COIN_01,          true),
            CHECK(RC_JRB_BLUE_COIN_02,          true),
            CHECK(RC_JRB_BLUE_COIN_03,          true),
            CHECK(RC_JRB_BLUE_COIN_04,          true),
            CHECK(RC_JRB_BLUE_COIN_05,          true),
            CHECK(RC_JRB_BLUE_COIN_06,          true),
            CHECK(RC_JRB_RED_COIN_01,           true),
            CHECK(RC_JRB_RED_COIN_02,           true),
            CHECK(RC_JRB_RED_COIN_03,           true),
            CHECK(RC_JRB_RED_COIN_04,           true),
            CHECK(RC_JRB_RED_COIN_05,           true),
            CHECK(RC_JRB_RED_COIN_06,           true),
            CHECK(RC_JRB_RED_COIN_07,           HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01)),
            CHECK(RC_JRB_RED_COIN_08,           HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01)),
            CHECK(RC_JRB_STAR_01_SUNKEN_SHIP,   true),
            CHECK(RC_JRB_STAR_02_EEL,           HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01)),
            CHECK(RC_JRB_STAR_03_OCEAN_CAVE,    true),
            CHECK(RC_JRB_STAR_04_RED_COINS,     HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01)),
            CHECK(RC_JRB_STAR_05_STONE_PILLAR,  HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01)),
            CHECK(RC_JRB_STAR_06_JET_STREAM,    HAS_COURSE_STAR(COURSE_JRB, RA_ACT_01) && CAN_USE(METAL)),
            CHECK(RC_JRB_STAR_07_100_COIN,      true),
        },
    };
}, {});
// clang-format on