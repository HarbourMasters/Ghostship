#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_THI] = RandoRegion{ .regionName = "Tiny Huge Island", .levelId = LEVEL_THI,
        .checks = {
            CHECK(RC_THI_BLUE_COIN_01,          true),
            CHECK(RC_THI_BLUE_COIN_02,          true),
            CHECK(RC_THI_RED_COIN_01,           true),
            CHECK(RC_THI_RED_COIN_02,           true),
            CHECK(RC_THI_RED_COIN_03,           true),
            CHECK(RC_THI_RED_COIN_04,           true),
            CHECK(RC_THI_RED_COIN_05,           true),
            CHECK(RC_THI_RED_COIN_06,           true),
            CHECK(RC_THI_RED_COIN_07,           true),
            CHECK(RC_THI_RED_COIN_08,           true),
            CHECK(RC_THI_STAR_01_PIRANHA,       true),
            CHECK(RC_THI_STAR_02_ISLAND_TOP,    true),
            CHECK(RC_THI_STAR_03_KOOPA_REMATCH, CAN_ACCESS_ACT(COURSE_THI, RA_ACT_03)),
            CHECK(RC_THI_STAR_04_SECRETS,       true),
            CHECK(RC_THI_STAR_05_RED_COINS,     true),
            CHECK(RC_THI_STAR_06_WIGGLER,       true),
            CHECK(RC_THI_STAR_07_100_COIN,      true),
        },
    };
}, {});
// clang-format on