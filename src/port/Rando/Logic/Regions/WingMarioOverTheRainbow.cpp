#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_WMOTR] = RandoRegion{ .regionName = "Winged Mario over the Rainbow", .levelId = LEVEL_WMOTR,
        .checks = {
            CHECK(RC_WMOTR_RED_COIN_01,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_02,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_03,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_04,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_05,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_06,     CAN_USE(WING)),
            CHECK(RC_WMOTR_RED_COIN_07,     true),
            CHECK(RC_WMOTR_RED_COIN_08,     CAN_USE(WING)),
            CHECK(RC_WMOTR_STAR_RED_COINS,  CAN_USE(WING)),
        },
    };
}, {});
// clang-format on