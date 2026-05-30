#ifdef USE_NETWORKING
#include "Relay.h"
#include "SatellaClient.h"

#include "port/ShipInit.hpp"
#include "port/permissions/Permissions.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {

std::string PermKey(const std::string& channelId) {
    return "relay." + channelId;
}

struct ChannelState {
    Relay::ChannelDef def;
    bool liveRegistered = false;
};

std::mutex gMtx;
std::unordered_map<std::string, ChannelState> gChannels;

class RelaySubscriptionPacket : public Satella::IPacket {
  public:
    RelaySubscriptionPacket(std::string channelId, Relay::MessageCallback cb)
        : mChannelId(std::move(channelId)), mCallback(std::move(cb)), mRoute("/v1/relay/" + mChannelId + "/subscribe") {
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }
    bool IsSubscription() const override {
        return true;
    }

    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("Relay: subscribe '{}' returned status {}", mChannelId, status);
        } else {
            SPDLOG_INFO("Relay: subscribed to channel '{}'", mChannelId);
        }
    }

    void OnPush(int16_t status, uint8_t packetType, const std::string& body) override {
        if (status == 200 && packetType == 0x03 && mCallback) {
            mCallback(mChannelId, body.data(), static_cast<uint32_t>(body.size()));
        }
    }

  private:
    std::string mChannelId;
    Relay::MessageCallback mCallback;
    std::string mRoute;
};

void ActivateChannel(const std::string& channelId) {
    std::lock_guard<std::mutex> lock(gMtx);
    auto it = gChannels.find(channelId);
    if (it == gChannels.end() || it->second.liveRegistered) {
        return;
    }

    it->second.liveRegistered = true;
    auto packet = std::make_unique<RelaySubscriptionPacket>(channelId, it->second.def.onMessage);
    Satella::Client::Instance().RegisterLive(std::move(packet));
}

} // anonymous namespace

namespace Relay {

void RegisterChannel(const std::string& channelId, ChannelDef def) {
    Permissions::Register(PermKey(channelId), { def.modName + " \xe2\x80\x94 Relay Permission",
                                                def.modName + " wants to use the Satella relay channel:\n\n" + "    " +
                                                    channelId + "\n\n" + def.description + "\n\n" +
                                                    "Allow this mod to send and receive data over the network?" });

    {
        std::lock_guard<std::mutex> lock(gMtx);
        if (gChannels.contains(channelId)) {
            return;
        }
        gChannels[channelId] = { std::move(def), false };
    }

    if (GetPermission(channelId) == Permission::Allowed) {
        ActivateChannel(channelId);
    }
}

bool Send(const std::string& channelId, const char* data, uint32_t size) {
    if (GetPermission(channelId) != Permission::Allowed) {
        return false;
    }
    Satella::Client::Instance().SendRaw("/v1/relay/" + channelId + "/send", data, size);
    return true;
}

Permission GetPermission(const std::string& channelId) {
    switch (Permissions::Get(PermKey(channelId))) {
        case Permissions::State::Allowed: {
            return Permission::Allowed;
        }
        case Permissions::State::Denied: {
            return Permission::Denied;
        }
        default: {
            return Permission::Pending;
        }
    }
}

void RequestPermission(const std::string& channelId) {
    Permissions::Request(
        PermKey(channelId), [channelId]() { ActivateChannel(channelId); }, nullptr);
}

void Init() {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(gMtx);
        ids.reserve(gChannels.size());
        for (auto& [id, _] : gChannels) {
            ids.push_back(id);
        }
    }

    for (const auto& id : ids) {
        switch (GetPermission(id)) {
            case Permission::Allowed: {
                ActivateChannel(id);
                break;
            }
            case Permission::Pending: {
                RequestPermission(id);
                break;
            }
            case Permission::Denied: {
                break;
            }
        }
    }
}

} // namespace Relay

static RegisterShipInitFunc sRelayInit(Relay::Init);

#endif
