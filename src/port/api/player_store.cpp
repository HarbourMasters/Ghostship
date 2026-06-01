#include "player_store.h"

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
static constexpr size_t kMaxPubKeyLen = 64;
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
    return "player-store." + channelId;
}

static const char* ScopeName(C_PlayerStoreScope scope) {
    return scope == PLAYER_STORE_PUBLIC ? "public" : "private";
}

// ---------------------------------------------------------------------------

class PlayerStoreGetPacket : public Satella::IPacket {
    std::string mRoute;
    C_PlayerStoreGetCallback mCallback;
    std::string mKey;

  public:
    PlayerStoreGetPacket(const std::string& pubKeyHex, const std::string& channel, C_PlayerStoreScope scope,
                         const std::string& key, C_PlayerStoreGetCallback cb)
        : mRoute("/v1/store/" + pubKeyHex + "/" + channel + "/" + ScopeName(scope) + "/" + key + "/get"), mCallback(cb),
          mKey(key) {
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

class PlayerStoreSetPacket : public Satella::IPacket {
    std::string mRoute;
    std::vector<uint8_t> mBody;

  public:
    PlayerStoreSetPacket(const std::string& pubKeyHex, const std::string& channel, C_PlayerStoreScope scope,
                         const std::string& key, const char* data, uint32_t size) {
        mRoute = "/v1/store/" + pubKeyHex + "/" + channel + "/" + ScopeName(scope) + "/" + key + "/set";

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
            SPDLOG_WARN("PlayerStore: set returned status {}", status);
        }
    }

    const std::vector<uint8_t>& Body() const {
        return mBody;
    }
};

class PlayerStoreDeletePacket : public Satella::IPacket {
    std::string mRoute;
    std::vector<uint8_t> mSig;

  public:
    PlayerStoreDeletePacket(const std::string& pubKeyHex, const std::string& channel, C_PlayerStoreScope scope,
                            const std::string& key) {
        mRoute = "/v1/store/" + pubKeyHex + "/" + channel + "/" + ScopeName(scope) + "/" + key + "/delete";

        const std::string routeKey = channel + "/" + ScopeName(scope) + "/" + key;
        mSig = PlayerIdentity::Sign(reinterpret_cast<const uint8_t*>(routeKey.data()), routeKey.size());
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("PlayerStore: delete returned status {}", status);
        }
    }

    const std::vector<uint8_t>& Sig() const {
        return mSig;
    }
};

// ---------------------------------------------------------------------------

extern "C" void C_RegisterPlayerStore(const char* channelId, const C_PlayerStoreDef* def) {
    if (!def || !IsValidId(channelId, kMaxChannelIdLen)) {
        SPDLOG_WARN("C_RegisterPlayerStore: invalid channelId or def");
        return;
    }

    const std::string modName = SafeStr(def->mod_name, kMaxModNameLen);
    const std::string desc = SafeStr(def->description, kMaxDescLen);

    Permissions::Register(PermKey(channelId),
                          { modName + " \xe2\x80\x94 Player Store Permission",
                            modName + " wants to store player data for channel:\n\n    " + channelId + "\n\n" + desc +
                                "\n\nAllow this mod to read and write your player data?" });
}

extern "C" C_PlayerStorePermission C_PlayerStoreGetPermission(const char* channelId) {
    if (!IsValidId(channelId, kMaxChannelIdLen)) {
        return PLAYER_STORE_PERM_DENIED;
    }

    switch (Permissions::Get(PermKey(channelId))) {
        case Permissions::State::Allowed:
            return PLAYER_STORE_PERM_ALLOWED;
        case Permissions::State::Denied:
            return PLAYER_STORE_PERM_DENIED;
        default:
            return PLAYER_STORE_PERM_PENDING;
    }
}

extern "C" void C_PlayerStoreRequestPermission(const char* channelId) {
    if (!IsValidId(channelId, kMaxChannelIdLen)) {
        return;
    }

    Permissions::Request(PermKey(channelId), nullptr, nullptr);
}

extern "C" int C_PlayerStoreSet(const char* channelId, C_PlayerStoreScope scope, const char* key, const char* data,
                                uint32_t size) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_PlayerStoreSet: invalid channelId or key");
        return 0;
    }

    if (C_PlayerStoreGetPermission(channelId) != PLAYER_STORE_PERM_ALLOWED) {
        return 0;
    }

    if (!data || size == 0 || size > kMaxValueBytes) {
        SPDLOG_WARN("C_PlayerStoreSet: invalid data (size={})", size);
        return 0;
    }

    auto pub = PlayerIdentity::GetPublicKey();

    if (pub.empty()) {
        SPDLOG_WARN("C_PlayerStoreSet: player identity not initialized");
        return 0;
    }

    const std::string pubKeyHex = StringHelper::BytesToHex(pub);
    auto packet = std::make_unique<PlayerStoreSetPacket>(pubKeyHex, channelId, scope, key, data, size);
    const auto& body = packet->Body();

    Satella::Client::Instance().SendRaw(packet->GetRoute(), body.data(), body.size());
    return 1;
}

extern "C" int C_PlayerStoreGet(const char* pubKeyHex, const char* channelId, C_PlayerStoreScope scope, const char* key,
                                C_PlayerStoreGetCallback callback) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_PlayerStoreGet: invalid channelId or key");
        return 0;
    }

    if (!callback) {
        SPDLOG_WARN("C_PlayerStoreGet: null callback");
        return 0;
    }

    std::string resolvedPubKey;

    if (!pubKeyHex || pubKeyHex[0] == '\0') {
        if (scope == PLAYER_STORE_PRIVATE && C_PlayerStoreGetPermission(channelId) != PLAYER_STORE_PERM_ALLOWED) {
            return 0;
        }

        auto pub = PlayerIdentity::GetPublicKey();

        if (pub.empty()) {
            SPDLOG_WARN("C_PlayerStoreGet: player identity not initialized");
            return 0;
        }

        resolvedPubKey = StringHelper::BytesToHex(pub);
    } else {
        if (!IsValidId(pubKeyHex, kMaxPubKeyLen) || std::strlen(pubKeyHex) != kMaxPubKeyLen) {
            SPDLOG_WARN("C_PlayerStoreGet: invalid pubKeyHex");
            return 0;
        }

        resolvedPubKey = pubKeyHex;
    }

    Satella::Client::Instance().RegisterLive(
        std::make_unique<PlayerStoreGetPacket>(resolvedPubKey, channelId, scope, key, callback));
    return 1;
}

extern "C" int C_PlayerStoreDelete(const char* channelId, C_PlayerStoreScope scope, const char* key) {
    if (!IsValidId(channelId, kMaxChannelIdLen) || !IsValidId(key, kMaxKeyLen)) {
        SPDLOG_WARN("C_PlayerStoreDelete: invalid channelId or key");
        return 0;
    }

    if (C_PlayerStoreGetPermission(channelId) != PLAYER_STORE_PERM_ALLOWED) {
        return 0;
    }

    auto pub = PlayerIdentity::GetPublicKey();

    if (pub.empty()) {
        SPDLOG_WARN("C_PlayerStoreDelete: player identity not initialized");
        return 0;
    }

    const std::string pubKeyHex = StringHelper::BytesToHex(pub);
    auto packet = std::make_unique<PlayerStoreDeletePacket>(pubKeyHex, channelId, scope, key);
    const auto& sig = packet->Sig();

    Satella::Client::Instance().SendRaw(packet->GetRoute(), sig.data(), sig.size());
    return 1;
}

#else

extern "C" void C_RegisterPlayerStore(const char*, const C_PlayerStoreDef*) {
}
extern "C" C_PlayerStorePermission C_PlayerStoreGetPermission(const char*) {
    return PLAYER_STORE_PERM_DENIED;
}
extern "C" void C_PlayerStoreRequestPermission(const char*) {
}
extern "C" int C_PlayerStoreSet(const char*, C_PlayerStoreScope, const char*, const char*, uint32_t) {
    return 0;
}
extern "C" int C_PlayerStoreGet(const char*, const char*, C_PlayerStoreScope, const char*, C_PlayerStoreGetCallback) {
    return 0;
}
extern "C" int C_PlayerStoreDelete(const char*, C_PlayerStoreScope, const char*) {
    return 0;
}

#endif
