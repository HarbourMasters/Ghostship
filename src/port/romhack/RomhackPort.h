#pragma once

#include "Romhack.h"

namespace Romhack {

// Per-hack code port. Subclass it in specific/<Id>.cpp and list <Id> in ROMHACK_PORT_LIST
// (RomhackRegistration.cpp). All custom logic goes through events (REGISTER_LISTENER) and the port's own APIs.
class Port {
  public:
    virtual ~Port() = default;

    // Runs at the end of FinishInit's resource setup, before ShipInit, so enhancements that snapshot compiled
    // tables see the hack's values. Read the hack's romhack/<table> blobs from the archive here.
    virtual void ApplyDataTables() {
    }
    // Runs at ShipInit, once per launch. Switching romhacks needs a restart, so listeners never need unregistering.
    virtual void Init() {
    }
};

// The active hack's port, or null on vanilla and for hacks without code.
Port* ActivePort();

} // namespace Romhack
