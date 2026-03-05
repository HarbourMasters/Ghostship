#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_BITS] = RandoRegion{ .regionName = "Bowser in the Sky", .levelId = LEVEL_BITS,
        .checks = {
            CHECK(RC_BITS_RED_COIN_01, true),
            CHECK(RC_BITS_RED_COIN_02, true),
            CHECK(RC_BITS_RED_COIN_03, true),
            CHECK(RC_BITS_RED_COIN_04, true),
            CHECK(RC_BITS_RED_COIN_05, true),
            CHECK(RC_BITS_RED_COIN_06, true),
            CHECK(RC_BITS_RED_COIN_07, true),
            CHECK(RC_BITS_RED_COIN_08, true),
            //CHECK(RC_BITS_STAR_RED_COINS, true), TODO: Add missing check
        },
    };
}, {});
// clang-format on