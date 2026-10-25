#ifdef ENABLE_SCRIPTING
#include "port/api/channel_store.h"
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
    return "channel-store." + channel_id;
}

static std::string OwnKeyHex() {
    const auto pub = PlayerIdentity::GetPublicKey();
    return pub.empty() ? std::string() : StringHelper::BytesToHex(pub);
}

class ChannelStoreGetPacket : public Satella::IPacket {
  public:
    ChannelStoreGetPacket(const std::string& channel, std::string key, C_ChannelStoreGetCallback callback)
        : mRoute("/v1/channel-store/" + channel + "/" + key + "/get"), mKey(std::move(key)), mCallback(callback) {
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
    C_ChannelStoreGetCallback mCallback;
    OnlineApi::Caller mCaller;
};

extern "C" void C_RegisterChannelStoreSized(const char* channel_id, const C_ChannelStoreDef* def, uint32_t size) {
    const SizedDef<C_ChannelStoreDef> sized(def, size);
    if (sized.get() == nullptr || !OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        SPDLOG_WARN("C_RegisterChannelStore: invalid channel id or definition");
        return;
    }
    const std::string modName = OnlineApi::Clamp(sized.get()->mod_name, 64);
    const std::string description = OnlineApi::Clamp(sized.get()->description, 256);
    Permissions::Register(PermKey(channel_id),
                          { modName + ": shared online data",
                            modName + " wants to share data online with everyone in the channel\n\n    " + channel_id +
                                "\n\n" + description + "\n\nAllow this mod to write to the shared data?" });
}

extern "C" C_PermissionState C_ChannelStoreGetPermission(const char* channel_id) {
    if (!OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        return C_PERM_DENIED;
    }
    return OnlineApi::ToC(Permissions::Get(PermKey(channel_id)));
}

extern "C" void C_ChannelStoreRequestPermission(const char* channel_id) {
    if (OnlineApi::IsValidId(channel_id, kIdMax, kIdChars)) {
        Permissions::Request(PermKey(channel_id));
    }
}

extern "C" int C_ChannelStoreSet(const char* channel_id, const char* key, const char* data, uint32_t size) {
    if (!ValidIds(channel_id, key, "C_ChannelStoreSet") || C_ChannelStoreGetPermission(channel_id) != C_PERM_ALLOWED ||
        !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    if (data == nullptr || size == 0 || size > C_CHANNEL_STORE_VALUE_MAX) {
        SPDLOG_WARN("C_ChannelStoreSet: values are 1 to {} bytes, got {}", C_CHANNEL_STORE_VALUE_MAX, size);
        return 0;
    }
    const std::string pub = OwnKeyHex();
    if (pub.empty()) {
        return 0;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    std::vector<uint8_t> body = PlayerIdentity::Sign(bytes, size);
    body.insert(body.end(), bytes, bytes + size);
    Satella::Client::Instance().SendLatest("/v1/channel-store/" + pub + "/" + channel_id + "/" + key + "/set",
                                           std::move(body), [](int16_t status, const std::string&) {
                                               if (status != 200) {
                                                   SPDLOG_WARN("C_ChannelStoreSet: Satella returned status {}", status);
                                               }
                                           });
    return 1;
}

extern "C" int C_ChannelStoreGet(const char* channel_id, const char* key, C_ChannelStoreGetCallback callback) {
    if (!ValidIds(channel_id, key, "C_ChannelStoreGet") || callback == nullptr ||
        !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    Satella::Client::Instance().RegisterLive(std::make_unique<ChannelStoreGetPacket>(channel_id, key, callback));
    return 1;
}

extern "C" int C_ChannelStoreDelete(const char* channel_id, const char* key) {
    if (!ValidIds(channel_id, key, "C_ChannelStoreDelete") ||
        C_ChannelStoreGetPermission(channel_id) != C_PERM_ALLOWED || !Satella::Client::Instance().IsConnected()) {
        return 0;
    }
    const std::string pub = OwnKeyHex();
    if (pub.empty()) {
        return 0;
    }
    const std::string signedPart = std::string(channel_id) + "/" + key;
    const auto sig = PlayerIdentity::Sign(reinterpret_cast<const uint8_t*>(signedPart.data()), signedPart.size());
    // A set queued before this delete must not take the value of one made after it.
    Satella::Client::Instance().ClosePending("/v1/channel-store/" + pub + "/" + channel_id + "/" + key + "/set");
    Satella::Client::Instance().RegisterLive(std::make_unique<Satella::RequestPacket>(
        "/v1/channel-store/" + pub + "/" + channel_id + "/" + key + "/delete", sig));
    return 1;
}

#else

extern "C" void C_RegisterChannelStoreSized(const char*, const C_ChannelStoreDef*, uint32_t) {
}
extern "C" C_PermissionState C_ChannelStoreGetPermission(const char*) {
    return C_PERM_DENIED;
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
#endif
