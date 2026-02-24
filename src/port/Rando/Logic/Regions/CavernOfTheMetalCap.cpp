#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_COTMC] = RandoRegion{ .regionName = "Cavern of the Metal Cap", .levelId = LEVEL_COTMC,
        .checks = {
            CHECK(RC_COTMC_RED_COIN_01, CAN_USE(METAL)),
            CHECK(RC_COTMC_RED_COIN_02, true),
            CHECK(RC_COTMC_RED_COIN_03, CAN_USE(METAL)),
            CHECK(RC_COTMC_RED_COIN_04, true),
            CHECK(RC_COTMC_RED_COIN_05, CAN_USE(METAL)),
            CHECK(RC_COTMC_RED_COIN_06, true),
            CHECK(RC_COTMC_RED_COIN_07, CAN_USE(METAL)),
            CHECK(RC_COTMC_RED_COIN_08, true),
            CHECK(RC_COTMC_STAR_RED_COINS, CAN_USE(METAL)),
        },
    };
}, {});
// clang-format on