#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_WF] = RandoRegion{ .regionName = "Whomp's Fortress", .levelId = LEVEL_WF,
        .checks = {
            CHECK(RC_WF_BLUE_COIN_01, true),
            CHECK(RC_WF_BLUE_COIN_02, true),
            CHECK(RC_WF_BLUE_COIN_03, true),
            CHECK(RC_WF_BLUE_COIN_04, true),
            CHECK(RC_WF_RED_COIN_01, true),
            CHECK(RC_WF_RED_COIN_02, true),
            CHECK(RC_WF_RED_COIN_03, true),
            CHECK(RC_WF_RED_COIN_04, true),
            CHECK(RC_WF_RED_COIN_05, true),
            CHECK(RC_WF_RED_COIN_06, true),
            CHECK(RC_WF_RED_COIN_07, true),
            CHECK(RC_WF_RED_COIN_08, true),
            CHECK(RC_WF_STAR_01_WHOMPS_BLOCK, true),
            CHECK(RC_WF_STAR_02_FORTRESS_TOP, true),
            CHECK(RC_WF_STAR_03_WILD_BLUE, true),
            CHECK(RC_WF_STAR_04_RED_COINS, true),
            CHECK(RC_WF_STAR_05_CAGED_ISLAND, true),
            CHECK(RC_WF_STAR_06_WALL, true),
            CHECK(RC_WF_STAR_07_100_COIN, true),
        },
    };
}, {});
// clang-format on