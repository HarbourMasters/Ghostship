#ifndef __SWITCH__
#include "relay.h"
#include "port/net/Relay.h"
#include "spdlog/spdlog.h"

#include <cctype>
#include <cstring>

static constexpr size_t kMaxChannelIdLen = 128;
static constexpr size_t kMaxModNameLen = 64;
static constexpr size_t kMaxDescLen = 256;
static constexpr uint32_t kMaxSendBytes = 64u * 1024u; // 64 KB

static bool IsValidChannelId(const char* id) {
    if (!id || id[0] == '\0') {
        return false;
    }

    const size_t len = std::strlen(id);
    if (len > kMaxChannelIdLen) {
        return false;
    }

    if (id[0] == '.' || id[len - 1] == '.') {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        const char c = id[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-') {
            return false;
        }

        if (c == '.' && id[i + 1] == '.') {
            return false;
        }
    }

    return true;
}

static std::string SafeStr(const char* s, size_t maxLen) {
    if (!s) {
        return "";
    }

    const size_t len = std::strlen(s);
    return std::string(s, len < maxLen ? len : maxLen);
}

extern "C" void C_RegisterRelayChannel(const char* channelId, const C_RelayChannelDef* def) {
    if (!def) {
        SPDLOG_WARN("C_RegisterRelayChannel: def is NULL");
        return;
    }

    if (!IsValidChannelId(channelId)) {
        SPDLOG_WARN("C_RegisterRelayChannel: invalid channelId '{}'", channelId ? channelId : "(null)");
        return;
    }

    if (!def->on_message) {
        SPDLOG_WARN("C_RegisterRelayChannel: '{}' has no on_message callback", channelId);
        return;
    }

    Relay::ChannelDef cppDef;
    cppDef.modName = SafeStr(def->mod_name, kMaxModNameLen);
    cppDef.description = SafeStr(def->description, kMaxDescLen);

    C_RelayCallback cb = def->on_message;
    cppDef.onMessage = [cb](const std::string& id, const char* data, uint32_t size) { cb(id.c_str(), data, size); };

    Relay::RegisterChannel(channelId, std::move(cppDef));
}

extern "C" int C_RelaySend(const char* channelId, const char* data, uint32_t size) {
    if (!IsValidChannelId(channelId)) {
        SPDLOG_WARN("C_RelaySend: invalid channelId '{}'", channelId ? channelId : "(null)");
        return 0;
    }

    if (size == 0) {
        SPDLOG_WARN("C_RelaySend: '{}' called with size=0", channelId);
        return 0;
    }

    if (!data) {
        SPDLOG_WARN("C_RelaySend: '{}' data is NULL", channelId);
        return 0;
    }

    if (size > kMaxSendBytes) {
        SPDLOG_WARN("C_RelaySend: '{}' payload too large ({} > {} bytes)", channelId, size, kMaxSendBytes);
        return 0;
    }

    return Relay::Send(channelId, data, size) ? 1 : 0;
}

extern "C" C_RelayPermission C_RelayGetPermission(const char* channelId) {
    if (!IsValidChannelId(channelId)) {
        return RELAY_PERM_DENIED;
    }

    return static_cast<C_RelayPermission>(Relay::GetPermission(channelId));
}

extern "C" void C_RelayRequestPermission(const char* channelId) {
    if (!IsValidChannelId(channelId)) {
        SPDLOG_WARN("C_RelayRequestPermission: invalid channelId '{}'", channelId ? channelId : "(null)");
        return;
    }

    Relay::RequestPermission(channelId);
}
#endif
