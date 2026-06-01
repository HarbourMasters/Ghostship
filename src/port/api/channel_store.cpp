#include "channel_store.h"

#ifdef USE_NETWORKING
#include "port/net/SatellaClient.h"
#include "port/net/PlayerIdentity.h"
#include "port/permissions/Permissions.h"
#include "ship/utils/StringHelper.h"
#include "spdlog/spdlog.h"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

static constexpr size_t kMaxChannelIdLen = 128;
static constexpr size_t kMaxKeyLen = 128;
static constexpr size_t kMaxModNameLen = 64;
static constexpr size_t kMaxDescLen = 256;
static constexpr uint32_t kMaxValueBytes = 4u * 1024u;

static bool IsValidId(const char* s, size_t maxLen) {
    if (!s || s[0] == '\0') {
        return false;
    }

    const size_t len = std::strlen(s);

    if (len > maxLen) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        const char c = s[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-' && c != '@') {
            return false;
        }
    }

    return true;
}

static std::string SafeStr(const char* s, size_t maxLen) {
    if (!s) {
        return {};
    }
    const size_t len = std::strlen(s);
    return { s, len < maxLen ? len : maxLen };
}

static std::string PermKey(const std::string& channelId) {
    return "channel-store." + channelId;
}

// ---------------------------------------------------------------------------

class ChannelStoreGetPacket : public Satella::IPacket {
    std::string mRoute;
    C_ChannelStoreGetCallback mCallback;
    std::string mKey;

  public:
    ChannelStoreGetPacket(std::string channel, std::string key, C_ChannelStoreGetCallback cb)
        : mRoute("/v1/channel-store/" + channel + "/" + key + "/get"), mCallback(cb), mKey(std::move(key)) {
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    void OnResponse(int16_t status, const std::string& body) override {
        if (!mCallback) {
            return;
        }

        if (status == 200) {
            mCallback(mKey.c_str(), body.data(), static_cast<uint32_t>(body.size()));
        } else {
            mCallback(mKey.c_str(), nullptr, 0);
        }
    }
};

class ChannelStoreSetPacket : public Satella::IPacket {
    std::string mRoute;
    std::vector<uint8_t> mBody;

  public:
    ChannelStoreSetPacket(const std::string& pubKeyHex, const std::string& channel, const std::string& key,
                          const char* data, uint32_t size) {
        mRoute = "/v1/channel-store/" + pubKeyHex + "/" + channel + "/" + key + "/set";

        auto sig = PlayerIdentity::Sign(reinterpret_cast<const uint8_t*>(data), size);

        mBody.reserve(sig.size() + size);
        mBody.insert(mBody.end(), sig.begin(), sig.end());
        mBody.insert(mBody.end(), reinterpret_cast<const uint8_t*>(data),
                     reinterpret_cast<const uint8_t*>(data) + size);
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("ChannelStore: set returned status {}", status);
        }
    }

    const std::vector<uint8_t>& Body() const {
        return mBody;
    }
};

class ChannelStoreDeletePacket : public Satella::IPacket {
    std::string mRoute;
    std::vector<uint8_t> mSig;

  public:
    ChannelStoreDeletePacket(const std::string& pubKeyHex, const std::string& channel, const std::string& key) {
        mRoute = "/v1/channel-store/" + pubKeyHex + "/" + channel + "/" + key + "/delete";

        const std::string routeKey = channel + "/" + key;
        mSig = PlayerIdentity::Sign(reinterpret_cast<const uint8_t*>(routeKey.data()), routeKey.size());
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("ChannelStore: delete returned status {}", status);
        }
    }

    const std::vector<uint8_t>& Sig() const {
        return mSig;
    }
};

// ---------------------------------------------------------------------------

extern "C" void C_RegisterChannelStore(const char* channelId, const C_ChannelStoreDef* def) {
    if (!def || !IsValidId(channelId, kMaxChannelIdLen)) {
        SPDLOG_WARN("C_RegisterChannelStore: invalid channelId or def");
        return;
    }

    const std::string modName = SafeStr(def->mod_name, kMaxModNameLen);
    const std::string desc = SafeStr(def->description, kMaxDescLen);

    Permissions::Register(PermKey(channelId),
                          { modName + " \xe2\x80\x94 Channel Store Permission",
                            modName + " wants to store shared mod data for channel:\n\n    " + channelId + "\n\n" +
                                desc + "\n\nAllow this mod to read and write shared channel data?" });
}

extern "C" C_ChannelStorePermission C_ChannelStoreGetPermission(const char* channelId) {
    if (!IsValidId(channelId, kMaxChannelIdLen)) {
        return CHANNEL_STORE_PERM_DENIED;
    }

    switch (Permissions::Get(PermKey(channelId))) {
        case Permissions::State::Allowed:
            return CHANNEL_STORE_PERM_ALLOWED;
        case Permissions::State::Denied:
            return CHANNEL_STORE_PERM_DENIED;
        default:
            return CHANNEL_STORE_PERM_PENDING;
    }
}

extern "C" void C_ChannelStoreRequestPermission(const char* channelId) {
    if (!IsValidId(channelId, kMaxChannelIdLen)) {
        return;
    }

    Permissions::Request(PermKey(channelId), nullptr, nullptr);
}

extern "C" int C_ChannelStoreSet(const char* channelId, const char* key, const char* data, uint32_t size) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_ChannelStoreSet: invalid channelId or key");
        return 0;
    }

    if (C_ChannelStoreGetPermission(channelId) != CHANNEL_STORE_PERM_ALLOWED) {
        return 0;
    }

    if (!data || size == 0 || size > kMaxValueBytes) {
        SPDLOG_WARN("C_ChannelStoreSet: invalid data (size={})", size);
        return 0;
    }

    auto pub = PlayerIdentity::GetPublicKey();

    if (pub.empty()) {
        SPDLOG_WARN("C_ChannelStoreSet: player identity not initialized");
        return 0;
    }

    const std::string pubKeyHex = StringHelper::BytesToHex(pub);
    auto packet = std::make_unique<ChannelStoreSetPacket>(pubKeyHex, channelId, key, data, size);
    const auto& body = packet->Body();

    Satella::Client::Instance().SendRaw(packet->GetRoute(), body.data(), body.size());
    return 1;
}

extern "C" int C_ChannelStoreGet(const char* channelId, const char* key, C_ChannelStoreGetCallback callback) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_ChannelStoreGet: invalid channelId or key");
        return 0;
    }

    if (!callback) {
        SPDLOG_WARN("C_ChannelStoreGet: null callback");
        return 0;
    }

    Satella::Client::Instance().RegisterLive(std::make_unique<ChannelStoreGetPacket>(channelId, key, callback));
    return 1;
}

extern "C" int C_ChannelStoreDelete(const char* channelId, const char* key) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_ChannelStoreDelete: invalid channelId or key");
        return 0;
    }

    if (C_ChannelStoreGetPermission(channelId) != CHANNEL_STORE_PERM_ALLOWED) {
        return 0;
    }

    auto pub = PlayerIdentity::GetPublicKey();

    if (pub.empty()) {
        SPDLOG_WARN("C_ChannelStoreDelete: player identity not initialized");
        return 0;
    }

    const std::string pubKeyHex = StringHelper::BytesToHex(pub);
    auto packet = std::make_unique<ChannelStoreDeletePacket>(pubKeyHex, channelId, key);
    const auto& sig = packet->Sig();

    Satella::Client::Instance().SendRaw(packet->GetRoute(), sig.data(), sig.size());
    return 1;
}

#else

extern "C" void C_RegisterChannelStore(const char*, const C_ChannelStoreDef*) {
}
extern "C" C_ChannelStorePermission C_ChannelStoreGetPermission(const char*) {
    return CHANNEL_STORE_PERM_DENIED;
}
extern "C" void C_ChannelStoreRequestPermission(const char*) {
}
extern "C" int C_ChannelStoreSet(const char*, const char*, const char*, uint32_t) {
    return 0;
}
extern "C" int C_ChannelStoreGet(const char*, const char*, C_ChannelStoreGetCallback) {
    return 0;
}
extern "C" int C_ChannelStoreDelete(const char*, const char*) {
    return 0;
}

#endif
