#include <cstring>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "RomhackPort.h"
#include "port/ShipInit.hpp"

// To port a hack's code: add its config.yml entry (romhack.id), subclass Romhack::Port in specific/<Id>.cpp
// exposing Create<Id>Port(), and list <Id> here, e.g. #define ROMHACK_PORT_LIST(PORT) PORT(StarRoad).
// Hacks without a port still get their assets.
#define ROMHACK_PORT_LIST(PORT)

#define ROMHACK_PORT_DECL(id) std::unique_ptr<Romhack::Port> Create##id##Port();
ROMHACK_PORT_LIST(ROMHACK_PORT_DECL)

struct RomhackPortEntry {
    const char* id;
    std::unique_ptr<Romhack::Port> (*create)();
};

#define ROMHACK_PORT_ENTRY(id) { #id, Create##id##Port },
static const std::vector<RomhackPortEntry> kRomhackPorts = { ROMHACK_PORT_LIST(ROMHACK_PORT_ENTRY) };

namespace Romhack {

Port* ActivePort() {
    static bool sResolved = false;
    static std::unique_ptr<Port> sPort;
    if (sResolved) {
        return sPort.get();
    }
    const char* id = Romhack_GetId();
    if (id == nullptr) {
        return nullptr;
    }
    sResolved = true;
    for (const RomhackPortEntry& entry : kRomhackPorts) {
        if (std::strcmp(id, entry.id) == 0) {
            SPDLOG_INFO("Romhack: using the {} code port", id);
            sPort = entry.create();
            return sPort.get();
        }
    }
    SPDLOG_INFO("Romhack: {} has no code port; running its assets on vanilla logic", id);
    return nullptr;
}

} // namespace Romhack

static void RomhackPort_Init() {
    if (Romhack::Port* port = Romhack::ActivePort()) {
        port->Init();
    }
}

static RegisterShipInitFunc sRomhackInit(RomhackPort_Init);
