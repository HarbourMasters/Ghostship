#ifdef ENABLE_SCRIPTING
#include "port/api/player_store.h"
#include "port/api/OnlineApi.hpp"
#include "port/api/SizedDef.hpp"

#ifdef USE_NETWORKING
#include "port/net/PlayerIdentity.h"
#include "port/net/SatellaClient.h"

#include <ship/utils/StringHelper.h>
#include <spdlog/spdlog.h>

#include <vector>

static constexpr size_t kIdMax = 128;
static constexpr const char* kIdChars = "._-@";

static bool ValidIds(const char* channel_id, const char* key, const char* caller) {
    if (!OnlineApi::IsValidId(channel_id, kIdMax, kIdChars) || !OnlineApi::IsValidId(key, kIdMax, kIdChars)) {
        SPDLOG_WARN("{}: invalid channel id or key", caller);
        return false;
    }
    return true;
}

static std::string PermKey(const std::string& channel_id) {
    return "player-store." + channel_id;
}

static const char* ScopeName(C_PlayerStoreScope scope) {
    return scope == C_PLAYER_STORE_PUBLIC ? "public" : "private";
}

static std::string OwnKeyHex() {
    const auto pub = PlayerIdentity::GetPublicKey();
    return pub.empty() ? std::string() : StringHelper::BytesToHex(pub);
}

static std::string Route(const std::string& pub_key, const std::string& channel, C_PlayerStoreScope scope,
                         const std::string& key, const char* action) {
    return "/v1/store/" + pub_key + "/" + channel + "/" + ScopeName(scope) + "/" + key + "/" + action;
}

class PlayerStoreGetPacket : public Satella::IPacket {
  public:
    PlayerStoreGetPacket(std::string route, std::string key, C_PlayerStoreGetCallback callback)
        : mRoute(std::move(route)), mKey(std::move(key)), mCallback(callback) {
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    void OnResponse(int16_t status, const std::string& body) override {
        Satella::Client::Instance().RunOnGameThread([caller = mCaller, callback = mCallback, key = mKey,
                                                     found = status == 200, body] {
            caller.Call([&] {
                callback(key.c_str(), found ? body.data() : nullptr, found ? static_cast<uint32_t>(body.size()) : 0);
            });
        });
    }

  private:
    std::string mRoute;
    std::string mKey;
    C_PlayerStoreGetCallback mCallback;
    OnlineApi::Caller mCaller;
};

extern "C" void C_RegisterPlayerStoreSized(const char* channel_id, const C_PlayerStoreDef* def, uint32_t size) {
    const SizedDef<C_PlayerStoreDef> sized(def, size);
    if (sized.get() == nullptr || !OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        SPDLOG_WARN("C_RegisterPlayerStore: invalid channel id or definition");
        return;
    }
    const std::string modName = OnlineApi::Clamp(sized.get()->mod_name, 64);
    const std::string description = OnlineApi::Clamp(sized.get()->description, 256);
    Permissions::Register(PermKey(channel_id),
                          { modName + ": online player data",
                            modName + " wants to keep data for you online, in the channel\n\n    " + channel_id +
                                "\n\n" + description + "\n\nAllow this mod to read and write your online data?" });
}

extern "C" C_PermissionState C_PlayerStoreGetPermission(const char* channel_id) {
    if (!OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        return C_PERM_DENIED;
    }
    return OnlineApi::ToC(Permissions::Get(PermKey(channel_id)));
}

extern "C" void C_PlayerStoreRequestPermission(const char* channel_id) {
    if (OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        Permissions::Request(PermKey(channel_id));
    }
}

extern "C" int C_PlayerStoreSet(const char* channel_id, C_PlayerStoreScope scope, const char* key, const char* data,
                                uint32_t size) {
    if (!ValidIds(channel_id, key, "C_PlayerStoreSet") || C_PlayerStoreGetPermission(channel_id) != C_PERM_ALLOWED ||
        !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    if (data == nullptr || size == 0 || size > C_PLAYER_STORE_VALUE_MAX) {
        SPDLOG_WARN("C_PlayerStoreSet: values are 1 to {} bytes, got {}", C_PLAYER_STORE_VALUE_MAX, size);
        return 0;
    }
    const std::string pub = OwnKeyHex();
    if (pub.empty()) {
        return 0;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    std::vector<uint8_t> body = PlayerIdentity::Sign(bytes, size);
    body.insert(body.end(), bytes, bytes + size);
    Satella::Client::Instance().SendLatest(Route(pub, channel_id, scope, key, "set"), std::move(body),
                                           [](int16_t status, const std::string&) {
                                               if (status != 200) {
                                                   SPDLOG_WARN("C_PlayerStoreSet: Satella returned status {}", status);
                                               }
                                           });
    return 1;
}

extern "C" int C_PlayerStoreGet(const char* pub_key_hex, const char* channel_id, C_PlayerStoreScope scope,
                                const char* key, C_PlayerStoreGetCallback callback) {
    if (!ValidIds(channel_id, key, "C_PlayerStoreGet") || callback == nullptr ||
        !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    std::string pub;
    if (pub_key_hex == nullptr || pub_key_hex[0] == '\0') {
        if (scope == C_PLAYER_STORE_PRIVATE && C_PlayerStoreGetPermission(channel_id) != C_PERM_ALLOWED) {
            return 0;
        }
        pub = OwnKeyHex();
    } else if (strlen(pub_key_hex) == 64 && OnlineApi::IsValidId(pub_key_hex, 64, "")) {
        pub = pub_key_hex;
    } else {
        SPDLOG_WARN("C_PlayerStoreGet: a player key is 64 hex characters");
        return 0;
    }
    if (pub.empty()) {
        return 0;
    }
    Satella::Client::Instance().RegisterLive(
        std::make_unique<PlayerStoreGetPacket>(Route(pub, channel_id, scope, key, "get"), key, callback));
    return 1;
}

extern "C" int C_PlayerStoreDelete(const char* channel_id, C_PlayerStoreScope scope, const char* key) {
    if (!ValidIds(channel_id, key, "C_PlayerStoreDelete") || C_PlayerStoreGetPermission(channel_id) != C_PERM_ALLOWED ||
        !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    const std::string pub = OwnKeyHex();
    if (pub.empty()) {
        return 0;
    }
    const std::string signedPart = std::string(channel_id) + "/" + ScopeName(scope) + "/" + key;
    const auto sig = PlayerIdentity::Sign(reinterpret_cast<const uint8_t*>(signedPart.data()), signedPart.size());
    // A set queued before this delete must not take the value of one made after it.
    Satella::Client::Instance().ClosePending(Route(pub, channel_id, scope, key, "set"));
    Satella::Client::Instance().RegisterLive(
        std::make_unique<Satella::RequestPacket>(Route(pub, channel_id, scope, key, "delete"), sig));
    return 1;
}

#else

extern "C" void C_RegisterPlayerStoreSized(const char*, const C_PlayerStoreDef*, uint32_t) {
}
extern "C" C_PermissionState C_PlayerStoreGetPermission(const char*) {
    return C_PERM_DENIED;
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
#endif
