#pragma once

#include "port/api/ModOwner.hpp"
#include "port/api/permissions.h"
#include "port/permissions/Permissions.h"

#include <cctype>
#include <cstring>
#include <memory>
#include <string>

namespace OnlineApi {

// Letters, digits and the extra characters given, up to max_len, without leading, trailing or doubled dots.
inline bool IsValidId(const char* id, size_t max_len, const char* extra) {
    if (id == nullptr || id[0] == '\0') {
        return false;
    }
    const size_t len = strlen(id);
    if (len > max_len || id[0] == '.' || id[len - 1] == '.') {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        const char c = id[i];
        if (!isalnum(static_cast<unsigned char>(c)) && strchr(extra, c) == nullptr) {
            return false;
        }
        if (c == '.' && id[i + 1] == '.') {
            return false;
        }
    }
    return true;
}

inline std::string Clamp(const char* s, size_t max_len) {
    if (s == nullptr) {
        return {};
    }
    return std::string(s, strnlen(s, max_len));
}

inline C_PermissionState ToC(Permissions::State state) {
    switch (state) {
        case Permissions::State::Allowed:
            return C_PERM_ALLOWED;
        case Permissions::State::Denied:
            return C_PERM_DENIED;
        default:
            return C_PERM_PENDING;
    }
}

// The mod making a call, so a later callback runs as that mod and is skipped once it has unloaded.
struct Caller {
    std::string mod = ModOwner::Current();
    std::shared_ptr<const bool> alive = ModOwner::Alive(mod);

    template <typename F> void Call(F&& f) const {
        if (*alive) {
            ModOwner::Scope scope(mod);
            f();
        }
    }
};

} // namespace OnlineApi
