#include "src/port/ShipInit.hpp"
#include "src/port/Rando/Logic/Logic.h"

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_LEVEL_SSL] = RandoRegion{ .regionName = "Shifting Sand Land", .levelId = LEVEL_SSL,
        .checks = {
            CHECK(RC_SSL_BLUE_COIN_01,              true),
            CHECK(RC_SSL_BLUE_COIN_02,              true),
            CHECK(RC_SSL_BLUE_COIN_03,              true),
            CHECK(RC_SSL_RED_COIN_01,               CAN_USE(WING)),
            CHECK(RC_SSL_RED_COIN_02,               CAN_USE(WING)),
            CHECK(RC_SSL_RED_COIN_03,               CAN_USE(WING)),
            CHECK(RC_SSL_RED_COIN_04,               CAN_USE(WING)),
            CHECK(RC_SSL_RED_COIN_05,               true),
            CHECK(RC_SSL_RED_COIN_06,               true),
            CHECK(RC_SSL_RED_COIN_07,               true),
            CHECK(RC_SSL_RED_COIN_08,               true),
            CHECK(RC_SSL_STAR_01_BIG_BIRD,          true),
            CHECK(RC_SSL_STAR_02_PYRAMID_TOP,       true),
            CHECK(RC_SSL_STAR_03_PYRAMID_INSIDE,    true),
            CHECK(RC_SSL_STAR_04_FOUR_PILLARS,      CAN_USE(WING)),
            CHECK(RC_SSL_STAR_05_RED_COINS,         true),
            CHECK(RC_SSL_STAR_06_PYRAMID_PUZZLE,    true),
            CHECK(RC_SSL_STAR_07_100_COIN,          true),
        },
    };
}, {});
// clang-format on