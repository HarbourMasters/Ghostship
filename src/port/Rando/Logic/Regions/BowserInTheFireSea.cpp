#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_BITFS] = RandoRegion{ .regionName = "Bowser in the Fire Sea", .levelId = LEVEL_BITFS,
        .checks = {
            CHECK(RC_BITFS_RED_COIN_01,     true),
            CHECK(RC_BITFS_RED_COIN_02,     true),
            CHECK(RC_BITFS_RED_COIN_03,     true),
            CHECK(RC_BITFS_RED_COIN_04,     true),
            CHECK(RC_BITFS_RED_COIN_05,     true),
            CHECK(RC_BITFS_RED_COIN_06,     true),
            CHECK(RC_BITFS_RED_COIN_07,     true),
            CHECK(RC_BITFS_RED_COIN_08,     true),
            CHECK(RC_BITFS_STAR_RED_COINS,  true),
        },
    };
}, {});
// clang-format on