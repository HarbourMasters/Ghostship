#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_CASTLE] = RandoRegion{ .regionName = "Castle Interior", .levelId = LEVEL_CASTLE,
        .checks = {
            CHECK(RC_CASTLE_STAR_01_TOAD_BASEMENT, true),
            CHECK(RC_CASTLE_STAR_02_TOAD_2ND_FLOOR, true),
            CHECK(RC_CASTLE_STAR_03_TOAD_3RD_FLOOR, true),
            CHECK(RC_CASTLE_STAR_04_MIPS_FIRST, true),
            CHECK(RC_CASTLE_STAR_05_MIPS_SECOND, true),
        },
    };
}, {});
// clang-format on