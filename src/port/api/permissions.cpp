#ifndef __SWITCH__
#include "permissions.h"
#include "port/permissions/Permissions.h"

extern "C" void C_RegisterPermission(const char* key, const char* title, const char* message) {
    if (!key) {
        return;
    }

    Permissions::Register(key, { title ? title : "", message ? message : "" });
}

extern "C" C_PermissionState C_GetPermission(const char* key) {
    if (!key) {
        return PERM_DENIED;
    }

    switch (Permissions::Get(key)) {
        case Permissions::State::Allowed: {
            return PERM_ALLOWED;
        }
        case Permissions::State::Denied: {
            return PERM_DENIED;
        }
        default: {
            return PERM_PENDING;
        }
    }
}

extern "C" void C_RequestPermission(const char* key, C_PermissionCallback on_allow, C_PermissionCallback on_deny) {
    if (!key) {
        return;
    }

    Permissions::Request(key, on_allow ? std::function<void()>(on_allow) : nullptr,
                         on_deny ? std::function<void()>(on_deny) : nullptr);
}
#endif
