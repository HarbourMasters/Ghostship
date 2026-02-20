#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_BOB] = RandoRegion{ .regionName = "Bob Omb Battlefield", .levelId = LEVEL_BOB,
        .checks = {
            CHECK(RC_BOB_RED_COIN_01, true),
            CHECK(RC_BOB_RED_COIN_02, true),
            CHECK(RC_BOB_RED_COIN_03, true),
            CHECK(RC_BOB_RED_COIN_04, true),
            CHECK(RC_BOB_RED_COIN_05, true),
            CHECK(RC_BOB_RED_COIN_06, true),
            CHECK(RC_BOB_RED_COIN_07, true),
            CHECK(RC_BOB_RED_COIN_08, true),
            CHECK(RC_BOB_STAR_01_KING_BOBOMB, true),
            CHECK(RC_BOB_STAR_02_KOOPA_RACE, HAS_COURSE_STAR(COURSE_BOB, RA_ACT_01)),
            CHECK(RC_BOB_STAR_03_ISLAND, HAS_COURSE_STAR(COURSE_BOB, RA_ACT_02)),
            CHECK(RC_BOB_STAR_04_RED_COINS, true),
            CHECK(RC_BOB_STAR_05_WINGS, HAS_UNLOCKED_CAP(WING)),
            CHECK(RC_BOB_STAR_06_CHAIN_CHOMP, true),
            CHECK(RC_BOB_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on