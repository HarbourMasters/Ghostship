#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_CASTLE] = RandoRegion{ .regionName = "Castle Interior", .levelId = LEVEL_CASTLE,
        .checks = {
            CHECK(RC_CASTLE_STAR_01_TOAD_BASEMENT,  CAN_USE(KEY_1)),
            CHECK(RC_CASTLE_STAR_02_TOAD_2ND_FLOOR, CAN_USE(KEY_2)),
            CHECK(RC_CASTLE_STAR_03_TOAD_3RD_FLOOR, CAN_USE(KEY_2) && HAS_TARGET_STARS(50)),
            CHECK(RC_CASTLE_STAR_04_MIPS_FIRST,     CAN_USE(KEY_1) && HAS_TARGET_STARS(15)),
            CHECK(RC_CASTLE_STAR_05_MIPS_SECOND,    CAN_USE(KEY_1) && HAS_TARGET_STARS(50)),
        },
        .connections = {
            CONNECTION(RR_LEVEL_BOB, CAN_ACCESS_ENTRANCE(RR_LEVEL_BOB)),
        },
    };
}, {});
// clang-format on