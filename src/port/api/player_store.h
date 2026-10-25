#pragma once

#include <stdint.h>

#include "api.h"
#include "port/api/permissions.h"

// Values kept online for this player, under a channel and a key, signed with the player's own key. Private values
// only this player reads; public ones anyone can read by the player's key. Writing, and reading private values, need
// the player's permission for the channel. Only in builds with Satella.

#define C_PLAYER_STORE_VALUE_MAX (4 * 1024)

typedef enum {
    C_PLAYER_STORE_PRIVATE,
    C_PLAYER_STORE_PUBLIC,
} C_PlayerStoreScope;

typedef struct C_PlayerStoreDef_s {
    const char* mod_name;    // shown in the permission prompt
    const char* description; // one sentence telling the player what is stored
} C_PlayerStoreDef;

// Runs on the game thread with the value, or with data NULL and size 0 when there is none.
typedef void (*C_PlayerStoreGetCallback)(const char* key, const char* data, uint32_t size);

// Registers the channel's permission prompt. Channel ids and keys are letters, digits, '.', '_', '-' and '@', up to
// 128 characters.
#define C_RegisterPlayerStore(id, def) C_RegisterPlayerStoreSized((id), (def), sizeof(*(def)))
GS_API void C_RegisterPlayerStoreSized(const char* channel_id, const C_PlayerStoreDef* def, uint32_t size);
GS_API C_PermissionState C_PlayerStoreGetPermission(const char* channel_id);
GS_API void C_PlayerStoreRequestPermission(const char* channel_id);
// Stores up to C_PLAYER_STORE_VALUE_MAX bytes. Returns FALSE when not allowed or not connected.
GS_API int C_PlayerStoreSet(const char* channel_id, C_PlayerStoreScope scope, const char* key, const char* data,
                            uint32_t size);
// Reads a value. pub_key_hex is another player's key in hex (C_PlayerPublicKey in port/api/online.h), NULL for this
// player's own values.
// Returns FALSE when the request can't be sent.
GS_API int C_PlayerStoreGet(const char* pub_key_hex, const char* channel_id, C_PlayerStoreScope scope, const char* key,
                            C_PlayerStoreGetCallback callback);
GS_API int C_PlayerStoreDelete(const char* channel_id, C_PlayerStoreScope scope, const char* key);
