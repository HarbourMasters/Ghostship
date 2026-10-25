#pragma once

#include <stdint.h>

#include "api.h"
#include "port/api/permissions.h"

// Values kept online for everyone running a channel, under keys. Anyone reads them; writing needs the player's
// permission for the channel and is signed with their key. Only in builds with Satella.

#define C_CHANNEL_STORE_VALUE_MAX (4 * 1024)

typedef struct C_ChannelStoreDef_s {
    const char* mod_name;    // shown in the permission prompt
    const char* description; // one sentence telling the player what is stored
} C_ChannelStoreDef;

// Runs on the game thread with the value, or with data NULL and size 0 when there is none.
typedef void (*C_ChannelStoreGetCallback)(const char* key, const char* data, uint32_t size);

// Registers the channel's permission prompt. Channel ids and keys are letters, digits, '.', '_', '-' and '@', up to
// 128 characters.
#define C_RegisterChannelStore(id, def) C_RegisterChannelStoreSized((id), (def), sizeof(*(def)))
GS_API void C_RegisterChannelStoreSized(const char* channel_id, const C_ChannelStoreDef* def, uint32_t size);
GS_API C_PermissionState C_ChannelStoreGetPermission(const char* channel_id);
GS_API void C_ChannelStoreRequestPermission(const char* channel_id);
// Stores up to C_CHANNEL_STORE_VALUE_MAX bytes. Returns FALSE when not allowed or not connected.
GS_API int C_ChannelStoreSet(const char* channel_id, const char* key, const char* data, uint32_t size);
// Reads a value; needs no permission. Returns FALSE when the request can't be sent.
GS_API int C_ChannelStoreGet(const char* channel_id, const char* key, C_ChannelStoreGetCallback callback);
GS_API int C_ChannelStoreDelete(const char* channel_id, const char* key);
