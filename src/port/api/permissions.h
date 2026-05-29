#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

typedef enum {
    PERM_PENDING = 0,
    PERM_ALLOWED = 1,
    PERM_DENIED  = 2,
} C_PermissionState;

typedef void (*C_PermissionCallback)(void);

// Register a permission key and the text shown to the user.
// Idempotent: safe to call multiple times with the same key.
extern_s void C_RegisterPermission(const char* key, const char* title, const char* message);

// Returns the current persisted state for the key.
extern_s C_PermissionState C_GetPermission(const char* key);

// Shows the permission dialog if state is still Pending.
// on_allow / on_deny may be NULL.
extern_s void C_RequestPermission(const char* key,
                                  C_PermissionCallback on_allow,
                                  C_PermissionCallback on_deny);
