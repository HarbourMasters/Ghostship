#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_TTM] = RandoRegion{ .regionName = "Tall Tall Mountain", .levelId = LEVEL_TTM,
        .checks = {
            CHECK(RC_TTM_RED_COIN_01, true),
            CHECK(RC_TTM_RED_COIN_02, true),
            CHECK(RC_TTM_RED_COIN_03, true),
            CHECK(RC_TTM_RED_COIN_04, true),
            CHECK(RC_TTM_RED_COIN_05, true),
            CHECK(RC_TTM_RED_COIN_06, true),
            CHECK(RC_TTM_RED_COIN_07, true),
            CHECK(RC_TTM_RED_COIN_08, true),
            CHECK(RC_TTM_STAR_01_MOUNTAIN_SCALE, true),
            CHECK(RC_TTM_STAR_02_MONKEY_CAGE, true),
            CHECK(RC_TTM_STAR_03_RED_COINS, true),
            CHECK(RC_TTM_STAR_04_MOUNTAINSIDE, true),
            CHECK(RC_TTM_STAR_05_BRIDGE_VIEW, true),
            CHECK(RC_TTM_STAR_06_MUSHROOM, true),
            CHECK(RC_TTM_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on