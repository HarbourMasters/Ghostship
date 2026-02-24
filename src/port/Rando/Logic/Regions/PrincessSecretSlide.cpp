#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_PSS] = RandoRegion{ .regionName = "Princess's Secret Slide", .levelId = LEVEL_PSS,
        .checks = {
            CHECK(RC_PSS_BLUE_COIN_01,          true),
            CHECK(RC_PSS_BLUE_COIN_02,          true),
            CHECK(RC_PSS_BLUE_COIN_03,          true),
            CHECK(RC_PSS_BLUE_COIN_04,          true),
            CHECK(RC_PSS_BLUE_COIN_05,          true),
            CHECK(RC_PSS_BLUE_COIN_06,          true),
            CHECK(RC_PSS_STAR_01_BOX,           true),
            CHECK(RC_PSS_STAR_02_SPEED_TIME,    true),
        },
    };
}, {});
// clang-format on