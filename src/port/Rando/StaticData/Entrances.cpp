#include "StaticData.h"
#include "port/ShipUtils.h"

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

extern "C" {
#include "game/area.h"
}

namespace Rando {

namespace StaticData {
// std::array<std::string, RC_MAX> CheckNames = std::array<std::string, RC_MAX>();

#define RE(id, destId, type, death)      \
    {                                    \
        id, {                            \
            id, #id, destId, type, death \
        }                                \
    }

// clang-format off
std::map<RandoEntranceId, RandoStaticEntrance> Entrances = {
    RE(RE_UNKNOWN,      LEVEL_UNKNOWN_1,    RETYPE_NONE,        WARP_NODE_00),
    RE(RE_BBH,          LEVEL_BBH,          RETYPE_PAINTING,    WARP_NODE_0B),
    RE(RE_BITDW,        LEVEL_BITDW,        RETYPE_BOWSER,      WARP_NODE_25),
    RE(RE_BITFS,        LEVEL_BITFS,        RETYPE_BOWSER,      WARP_NODE_68),
    RE(RE_BITS,         LEVEL_BITS,         RETYPE_BOWSER,      WARP_NODE_6B),
    RE(RE_BOB,          LEVEL_BOB,          RETYPE_PAINTING,    WARP_NODE_64),
    RE(RE_CCM,          LEVEL_CCM,          RETYPE_PAINTING,    WARP_NODE_65),
    RE(RE_COTMC,        LEVEL_COTMC,        RETYPE_CAP,         WARP_NODE_66),
    RE(RE_DDD,          LEVEL_DDD,          RETYPE_PAINTING,    WARP_NODE_67),
    RE(RE_HMC,          LEVEL_HMC,          RETYPE_PAINTING,    WARP_NODE_66),
    RE(RE_JRB,          LEVEL_JRB,          RETYPE_PAINTING,    WARP_NODE_67),
    RE(RE_LLL,          LEVEL_LLL,          RETYPE_PAINTING,    WARP_NODE_64),
    RE(RE_PSS,          LEVEL_PSS,          RETYPE_SECRET,      WARP_NODE_23),
    RE(RE_RR,           LEVEL_RR,           RETYPE_PAINTING,    WARP_NODE_6C),
    RE(RE_SA,           LEVEL_SA,           RETYPE_SECRET,      WARP_NODE_28),
    RE(RE_SL,           LEVEL_SL,           RETYPE_PAINTING,    WARP_NODE_68),
    RE(RE_SSL,          LEVEL_SSL,          RETYPE_PAINTING,    WARP_NODE_65),
    RE(RE_THI,          LEVEL_THI,          RETYPE_PAINTING,    WARP_NODE_69),
    RE(RE_TOTWC,        LEVEL_TOTWC,        RETYPE_CAP,         WARP_NODE_23),
    RE(RE_TTC,          LEVEL_TTC,          RETYPE_PAINTING,    WARP_NODE_67),
    RE(RE_TTM,          LEVEL_TTM,          RETYPE_PAINTING,    WARP_NODE_66),
    RE(RE_VCUTM,        LEVEL_VCUTM,        RETYPE_CAP,         WARP_NODE_06),
    RE(RE_WDW,          LEVEL_WDW,          RETYPE_PAINTING,    WARP_NODE_64),
    RE(RE_WF,           LEVEL_WF,           RETYPE_PAINTING,    WARP_NODE_66),
    RE(RE_WMOTR,        LEVEL_WMOTR,        RETYPE_SECRET,      WARP_NODE_0A),
};

// clang-format on

} // namespace StaticData
} // namespace Rando