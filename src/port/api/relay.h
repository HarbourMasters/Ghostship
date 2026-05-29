#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

typedef enum {
    RELAY_PERM_PENDING = 0, // user has not been asked yet
    RELAY_PERM_ALLOWED = 1, // user granted access
    RELAY_PERM_DENIED  = 2, // user denied access
} C_RelayPermission;

// Called when a message arrives on the channel.
// `data` points to the raw payload; `size` is its byte length.
typedef void (*C_RelayCallback)(const char* channelId, const char* data, uint32_t size);

// Passed to C_RegisterRelayChannel.
typedef struct {
    const char*     mod_name;    // displayed in the permission dialog
    const char*     description; // one-sentence explanation shown to the user
    C_RelayCallback on_message;  // called on every inbound message
} C_RelayChannelDef;

// Register a channel. Duplicate ids are silently ignored.
// Safe to call from any C init function before or after engine startup.
extern_s void C_RegisterRelayChannel(const char* channelId, const C_RelayChannelDef* def);

// Send binary data on a channel.
// Returns 1 on success, 0 if not permitted or not connected.
extern_s int  C_RelaySend(const char* channelId, const char* data, uint32_t size);

// Returns the current permission state for the channel.
extern_s C_RelayPermission C_RelayGetPermission(const char* channelId);

// Show the permission prompt for a channel that is still PENDING.
// No-op if permission is already Allowed or Denied.
extern_s void C_RelayRequestPermission(const char* channelId);
