#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

typedef enum {
    PLAYER_STORE_PERM_PENDING = 0,
    PLAYER_STORE_PERM_ALLOWED = 1,
    PLAYER_STORE_PERM_DENIED  = 2,
} C_PlayerStorePermission;

typedef enum {
    PLAYER_STORE_PRIVATE = 0,
    PLAYER_STORE_PUBLIC  = 1,
} C_PlayerStoreScope;

typedef struct {
    const char* mod_name;
    const char* description;
} C_PlayerStoreDef;

typedef void (*C_PlayerStoreGetCallback)(const char* key, const char* data, uint32_t size);

// Register a channel and its permission prompt. Must be called before any
// Set/Delete/Get-private operations. Duplicate ids are silently ignored.
extern_s void C_RegisterPlayerStore(const char* channelId, const C_PlayerStoreDef* def);

extern_s C_PlayerStorePermission C_PlayerStoreGetPermission(const char* channelId);
extern_s void C_PlayerStoreRequestPermission(const char* channelId);

// Returns 1 if dispatched, 0 if not permitted / not connected.
extern_s int C_PlayerStoreSet(const char* channelId, C_PlayerStoreScope scope,
                               const char* key, const char* data, uint32_t size);

// Pass NULL for pubKeyHex to read your own data.
// Private scope requires permission; public reads are always open.
extern_s int C_PlayerStoreGet(const char* pubKeyHex, const char* channelId,
                               C_PlayerStoreScope scope, const char* key,
                               C_PlayerStoreGetCallback callback);

// Returns 1 if dispatched, 0 if not permitted / not connected.
extern_s int C_PlayerStoreDelete(const char* channelId, C_PlayerStoreScope scope,
                                  const char* key);
