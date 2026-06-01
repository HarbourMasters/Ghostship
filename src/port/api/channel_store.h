#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

typedef enum {
    CHANNEL_STORE_PERM_PENDING = 0,
    CHANNEL_STORE_PERM_ALLOWED = 1,
    CHANNEL_STORE_PERM_DENIED  = 2,
} C_ChannelStorePermission;

typedef struct {
    const char* mod_name;
    const char* description;
} C_ChannelStoreDef;

typedef void (*C_ChannelStoreGetCallback)(const char* key, const char* data, uint32_t size);

// Register a channel and its permission prompt. Must be called before any
// Set/Delete operations. Duplicate ids are silently ignored.
extern_s void C_RegisterChannelStore(const char* channelId, const C_ChannelStoreDef* def);

extern_s C_ChannelStorePermission C_ChannelStoreGetPermission(const char* channelId);
extern_s void C_ChannelStoreRequestPermission(const char* channelId);

// Returns 1 if dispatched, 0 if not permitted / not connected.
extern_s int C_ChannelStoreSet(const char* channelId, const char* key,
                                const char* data, uint32_t size);

// Get is unauthenticated and does not require permission.
extern_s int C_ChannelStoreGet(const char* channelId, const char* key,
                                C_ChannelStoreGetCallback callback);

// Returns 1 if dispatched, 0 if not permitted / not connected.
extern_s int C_ChannelStoreDelete(const char* channelId, const char* key);
