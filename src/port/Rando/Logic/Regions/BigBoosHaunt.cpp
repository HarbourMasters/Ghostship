#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_BBH] = RandoRegion{ .regionName = "Big Boo's Haunt", .levelId = LEVEL_BBH,
        .checks = {
            CHECK(RC_BBH_BLUE_COIN_01, true),
            CHECK(RC_BBH_BLUE_COIN_02, true),
            CHECK(RC_BBH_BLUE_COIN_03, true),
            CHECK(RC_BBH_BLUE_COIN_04, true),
            CHECK(RC_BBH_RED_COIN_01, true),
            CHECK(RC_BBH_RED_COIN_02, true),
            CHECK(RC_BBH_RED_COIN_03, true),
            CHECK(RC_BBH_RED_COIN_04, true),
            CHECK(RC_BBH_RED_COIN_05, true),
            CHECK(RC_BBH_RED_COIN_06, true),
            CHECK(RC_BBH_RED_COIN_07, true),
            CHECK(RC_BBH_RED_COIN_08, true),
            CHECK(RC_BBH_STAR_01_GHOST_HUNT, true),
            CHECK(RC_BBH_STAR_02_MERRY_GO_ROUND, true),
            CHECK(RC_BBH_STAR_03_HAUNTED_BOOKS, true),
            CHECK(RC_BBH_STAR_04_RED_COINS, true),
            CHECK(RC_BBH_STAR_05_BALCONY, true),
            CHECK(RC_BBH_STAR_06_SECRET_ROOM, true),
            CHECK(RC_BBH_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on