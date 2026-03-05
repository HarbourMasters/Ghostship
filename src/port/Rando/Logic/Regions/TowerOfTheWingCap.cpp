#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_TOTWC] = RandoRegion{ .regionName = "Tower of the Wing Cap", .levelId = LEVEL_TOTWC,
        .checks = {
            CHECK(RC_TOTWC_RED_COIN_01,     true),
            CHECK(RC_TOTWC_RED_COIN_02,     true),
            CHECK(RC_TOTWC_RED_COIN_03,     true),
            CHECK(RC_TOTWC_RED_COIN_04,     true),
            CHECK(RC_TOTWC_RED_COIN_05,     true),
            CHECK(RC_TOTWC_RED_COIN_06,     true),
            CHECK(RC_TOTWC_RED_COIN_07,     true),
            CHECK(RC_TOTWC_RED_COIN_08,     true),
            CHECK(RC_TOTWC_STAR_RED_COINS,  true),
        },
    };
}, {});
// clang-format on