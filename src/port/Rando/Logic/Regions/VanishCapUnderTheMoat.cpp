#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_VCUTM] = RandoRegion{ .regionName = "Vanish Cap Under the Moat", .levelId = LEVEL_VCUTM,
        .checks = {
            CHECK(RC_VCUTM_RED_COIN_01, true),
            CHECK(RC_VCUTM_RED_COIN_02, true),
            CHECK(RC_VCUTM_RED_COIN_03, true),
            CHECK(RC_VCUTM_RED_COIN_04, true),
            CHECK(RC_VCUTM_RED_COIN_05, true),
            CHECK(RC_VCUTM_RED_COIN_06, true),
            CHECK(RC_VCUTM_RED_COIN_07, true),
            CHECK(RC_VCUTM_RED_COIN_08, true),
            CHECK(RC_VCUTM_STAR_RED_COINS, true),
        },
    };
}, {});
// clang-format on