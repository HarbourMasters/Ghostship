#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_HMC] = RandoRegion{ .regionName = "Hazy Maze Cave", .levelId = LEVEL_HMC,
        .checks = {
            CHECK(RC_HMC_BLUE_COIN_01, true),
            CHECK(RC_HMC_BLUE_COIN_02, true),
            CHECK(RC_HMC_BLUE_COIN_03, true),
            CHECK(RC_HMC_BLUE_COIN_04, true),
            CHECK(RC_HMC_BLUE_COIN_05, true),
            CHECK(RC_HMC_BLUE_COIN_06, true),
            CHECK(RC_HMC_BLUE_COIN_07, true),
            CHECK(RC_HMC_RED_COIN_01, true),
            CHECK(RC_HMC_RED_COIN_02, true),
            CHECK(RC_HMC_RED_COIN_03, true),
            CHECK(RC_HMC_RED_COIN_04, true),
            CHECK(RC_HMC_RED_COIN_05, true),
            CHECK(RC_HMC_RED_COIN_06, true),
            CHECK(RC_HMC_RED_COIN_07, true),
            CHECK(RC_HMC_RED_COIN_08, true),
            CHECK(RC_HMC_STAR_01_BEAST, true),
            CHECK(RC_HMC_STAR_02_RED_COINS, true),
            CHECK(RC_HMC_STAR_03_METAL_HEAD, true),
            CHECK(RC_HMC_STAR_04_TOXIC_MAZE, true),
            CHECK(RC_HMC_STAR_05_EMERGENCY_EXIT, true),
            CHECK(RC_HMC_STAR_06_ROLLING_ROCKS, true),
            CHECK(RC_HMC_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on