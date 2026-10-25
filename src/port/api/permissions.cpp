#ifdef ENABLE_SCRIPTING
#include "port/api/permissions.h"
#include "port/api/OnlineApi.hpp"
#include "port/permissions/Permissions.h"

extern "C" void C_RegisterPermission(const char* key, const char* title, const char* message) {
    if (key == nullptr) {
        return;
    }
    Permissions::Register(key, { title != nullptr ? title : "", message != nullptr ? message : "" });
}

extern "C" C_PermissionState C_GetPermission(const char* key) {
    return key != nullptr ? OnlineApi::ToC(Permissions::Get(key)) : C_PERM_DENIED;
}

extern "C" void C_RequestPermission(const char* key, C_PermissionCallback on_allow, C_PermissionCallback on_deny) {
    if (key == nullptr) {
        return;
    }
    const OnlineApi::Caller caller;
    Permissions::Request(
        key,
        [caller, on_allow] {
            if (on_allow != nullptr) {
                caller.Call(on_allow);
            }
        },
        [caller, on_deny] {
            if (on_deny != nullptr) {
                caller.Call(on_deny);
            }
        });
}
#endif
