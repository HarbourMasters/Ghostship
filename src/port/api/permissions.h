#pragma once

#include <stdint.h>

#include "api.h"

// Things the player allows or denies once, such as a mod going online. The game asks with an Allow / Deny prompt and
// remembers the answer.

typedef enum {
    C_PERM_PENDING, // not asked yet
    C_PERM_ALLOWED,
    C_PERM_DENIED,
} C_PermissionState;

typedef void (*C_PermissionCallback)(void);

// Sets the title and text of the prompt for key, e.g. "mymod.camera". Registering a key again replaces its prompt.
GS_API void C_RegisterPermission(const char* key, const char* title, const char* message);
GS_API C_PermissionState C_GetPermission(const char* key);
// Asks the player when the key is still pending. The callback for their answer runs after they choose; either may be
// NULL.
GS_API void C_RequestPermission(const char* key, C_PermissionCallback on_allow, C_PermissionCallback on_deny);
