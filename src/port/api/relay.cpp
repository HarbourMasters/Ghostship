#ifdef ENABLE_SCRIPTING
#include "port/api/relay.h"
#include "port/api/OnlineApi.hpp"
#include "port/api/SizedDef.hpp"

#ifdef USE_NETWORKING
#include "port/net/Relay.h"

#include <spdlog/spdlog.h>

static constexpr const char* kIdChars = "._-";

static bool ValidChannel(const char* id, const char* caller) {
    if (!OnlineApi::IsValidId(id, C_RELAY_ID_MAX, kIdChars)) {
        SPDLOG_WARN("{}: invalid channel id '{}'", caller, id != nullptr ? id : "(null)");
        return false;
    }
    return true;
}

extern "C" void C_RegisterRelayChannelSized(const char* channel_id, const C_RelayChannelDef* def, uint32_t size) {
    const SizedDef<C_RelayChannelDef> sized(def, size);
    if (!ValidChannel(channel_id, "C_RegisterRelayChannel") || sized.get() == nullptr) {
        return;
    }
    if (sized.get()->on_message == nullptr) {
        SPDLOG_WARN("C_RegisterRelayChannel: '{}' has no on_message", channel_id);
        return;
    }

    Relay::ChannelDef relayDef;
    relayDef.modName = OnlineApi::Clamp(sized.get()->mod_name, 64);
    relayDef.description = OnlineApi::Clamp(sized.get()->description, 256);
    relayDef.modVersion = OnlineApi::Clamp(sized.get()->mod_version, 32);
    relayDef.owner = ModOwner::Current();
    const C_RelayCallback callback = sized.get()->on_message;
    relayDef.onMessage = [callback](const std::string& id, const std::string& sender, const char* data,
                                    uint32_t length) { callback(id.c_str(), sender.c_str(), data, length); };
    Relay::RegisterChannel(channel_id, std::move(relayDef));
}

extern "C" int C_RelaySend(const char* channel_id, const char* data, uint32_t size) {
    if (!ValidChannel(channel_id, "C_RelaySend")) {
        return 0;
    }
    if (data == nullptr || size == 0 || size > C_RELAY_SEND_MAX) {
        SPDLOG_WARN("C_RelaySend: '{}' needs 1 to {} bytes, got {}", channel_id, C_RELAY_SEND_MAX, size);
        return 0;
    }
    return Relay::Send(channel_id, data, size) ? 1 : 0;
}

extern "C" C_PermissionState C_RelayGetPermission(const char* channel_id) {
    if (!OnlineApi::IsValidId(channel_id, C_RELAY_ID_MAX, kIdChars)) {
        return C_PERM_DENIED;
    }
    switch (Relay::GetPermission(channel_id)) {
        case Relay::Permission::Allowed:
            return C_PERM_ALLOWED;
        case Relay::Permission::Denied:
            return C_PERM_DENIED;
        default:
            return C_PERM_PENDING;
    }
}

extern "C" void C_RelayRequestPermission(const char* channel_id) {
    if (ValidChannel(channel_id, "C_RelayRequestPermission")) {
        Relay::RequestPermission(channel_id);
    }
}

#else

extern "C" void C_RegisterRelayChannelSized(const char*, const C_RelayChannelDef*, uint32_t) {
}
extern "C" int C_RelaySend(const char*, const char*, uint32_t) {
    return 0;
}
extern "C" C_PermissionState C_RelayGetPermission(const char*) {
    return C_PERM_DENIED;
}
extern "C" void C_RelayRequestPermission(const char*) {
}

#endif
#endif
