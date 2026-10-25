#pragma once

#include <stdint.h>

#include "api.h"
#include "port/api/permissions.h"

// Channels mods send binary messages on to every player running the same channel, through Satella. The player is
// asked once per channel before it is used. Only in builds with Satella; elsewhere sending fails and nothing arrives.

#define C_RELAY_ID_MAX 128
#define C_RELAY_SEND_MAX (64 * 1024)

// Runs on the game thread for every message on the channel. sender_key_hex is the sending player's key, proven to
// Satella, or "" when the sender has none.
typedef void (*C_RelayCallback)(const char* channel_id, const char* sender_key_hex, const char* data, uint32_t size);

typedef struct C_RelayChannelDef_s {
    const char* mod_name;    // shown in the permission prompt
    const char* description; // one sentence telling the player what the channel is for
    C_RelayCallback on_message;
    const char* mod_version; // keeps players on different versions of the mod apart, NULL for none
} C_RelayChannelDef;

// Registers a channel. Ids are letters, digits, '.', '_' and '-', up to C_RELAY_ID_MAX characters, e.g. "mymod.chat".
// Players the channel was allowed for start receiving right away; the others are asked once the game is up.
#define C_RegisterRelayChannel(id, def) C_RegisterRelayChannelSized((id), (def), sizeof(*(def)))
GS_API void C_RegisterRelayChannelSized(const char* channel_id, const C_RelayChannelDef* def, uint32_t size);
// Sends up to C_RELAY_SEND_MAX bytes. Returns FALSE when the channel isn't allowed or Satella isn't connected.
GS_API int C_RelaySend(const char* channel_id, const char* data, uint32_t size);
GS_API C_PermissionState C_RelayGetPermission(const char* channel_id);
// Asks the player for the channel when they haven't answered yet.
GS_API void C_RelayRequestPermission(const char* channel_id);
