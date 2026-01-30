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

#define RE(id, destId)      \
    {                       \
        id, {               \
            id, #id, destId \
        }                   \
    }

// clang-format off
std::map<RandoEntranceId, RandoStaticEntrance> Entrances = {
    RE(RE_UNKNOWN,      LEVEL_UNKNOWN_1 ),
    RE(RE_BOB,          LEVEL_BOB ),
    RE(RE_CCM,          LEVEL_CCM ),
    RE(RE_WF,           LEVEL_WF ),
    RE(RE_JRB,          LEVEL_JRB ),
    RE(RE_WDW,          LEVEL_WDW ),
    RE(RE_THI_LARGE,    LEVEL_THI ),
    RE(RE_TTM,          LEVEL_TTM ),
    RE(RE_TTC,          LEVEL_TTC ),
    RE(RE_SL,           LEVEL_SL ),
    RE(RE_THI_SMALL,    LEVEL_THI ),
    RE(RE_RR,           LEVEL_RR ),
    RE(RE_LLL,          LEVEL_LLL ),
    RE(RE_SSL,          LEVEL_SSL ),
    RE(RE_HMC,          LEVEL_HMC ),
    RE(RE_DDD,          LEVEL_DDD ),
};
// clang-format on

} // namespace StaticData
} // namespace Rando