#include "StaticData.h"

namespace Rando {

namespace StaticData {

#define RO(id, defaultValue)                             \
    {                                                    \
        id, {                                            \
            id, #id, "gRando.Options." #id, defaultValue \
        }                                                \
    }

// clang-format off
std::map<RandoOptionId, RandoStaticOption> Options = {
    RO(RO_LOGIC,                        RO_LOGIC_NO_LOGIC),
    RO(RO_SHUFFLE_COINS_BLUE,           RO_GENERIC_OFF),
    RO(RO_SHUFFLE_COINS_RED,            RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENTRANCES_BOWSER,     RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENTRANCES_CAP,        RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENTRANCES_PAINTING,   RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENTRANCES_SECRET,     RO_GENERIC_OFF),
    RO(RO_SHUFFLE_RED_COIN_STARS,       RO_GENERIC_OFF),
    RO(RO_SHUFFLE_STARS,                RO_GENERIC_OFF),
};

std::unordered_map<int32_t, const char*> logicOptions = {
    { RO_LOGIC_GLITCHLESS, "Glitchless" },
    { RO_LOGIC_NO_LOGIC, "No Logic" },
};
// clang-format on

RandoOptionId GetOptionIdFromName(const char* name) {
    for (auto& [randoOptionId, randoStaticOption] : Options) {
        if (strcmp(name, randoStaticOption.name) == 0) {
            return randoOptionId;
        }
    }
    return RO_MAX;
}

} // namespace StaticData

} // namespace Rando
