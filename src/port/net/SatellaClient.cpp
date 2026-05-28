#ifndef __SWITCH__
#include "port/net/SatellaClient.h"

#include "ship/Context.h"
#include "ship/security/Keystore.h"
#include "ship/utils/StringHelper.h"
#include "port/ui/Notification.h"
#include "spdlog/spdlog.h"

#include <nlohmann/json.hpp>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

static ix::WebSocket mWs;

namespace Satella {

static void writeU32LE(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

static int16_t readI16LE(const uint8_t* buf) {
    return static_cast<int16_t>(static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8));
}

static std::vector<uint8_t> buildRequest(const char* route) {
    std::vector<uint8_t> packet;

    const char magic[] = "HM64";
    for (int i = 0; i < 4; ++i) {
        packet.push_back(static_cast<uint8_t>(magic[i]));
    }

    packet.push_back(0x02); // PacketType::JSON

    const uint32_t routeLen = static_cast<uint32_t>(std::strlen(route));
    writeU32LE(packet, routeLen);
    for (uint32_t i = 0; i < routeLen; ++i) {
        packet.push_back(static_cast<uint8_t>(route[i]));
    }

    return packet;
}

Client& Client::Instance() {
    static Client instance;
    return instance;
}

Client::~Client() {
    if (mWs.getReadyState() != ix::ReadyState::Closed) {
        mWs.stop();
    }
    ix::uninitNetSystem();
}

void Client::Register(std::unique_ptr<IPacket> packet) {
    mPackets.push_back(std::move(packet));
}

void Client::OnMessage(const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            std::lock_guard<std::mutex> lock(mMtx);
            mConnected = true;
            mCv.notify_all();
            break;
        }
        case ix::WebSocketMessageType::Message: {
            const auto* data = reinterpret_cast<const uint8_t*>(msg->str.data());
            const size_t size = msg->str.size();
            constexpr size_t kHeaderSize = 5;

            if (size < kHeaderSize || data[0] != 'H' || data[1] != 'M') {
                SPDLOG_WARN("SatellaClient: malformed message (size={}, magic={:.2s})", size,
                            size >= 2 ? msg->str.data() : "??");
                break;
            }

            const int16_t  status = readI16LE(data + 3);
            const uint8_t  type   = data[2];
            const std::string body = (type != 0x01 && size > kHeaderSize)
                ? std::string(msg->str.begin() + kHeaderSize, msg->str.end())
                : std::string{};

            bool isResponse;
            {
                std::lock_guard<std::mutex> lock(mMtx);
                isResponse = mWaitingForResponse;
                if (isResponse) {
                    mResponseStatus = status;
                    mResponseBody   = body;
                    mResponseValid  = true;
                    mResponseReady  = true;
                    mCv.notify_all();
                }
            }

            if (!isResponse) {
                for (auto* sub : mSubscriptions) {
                    sub->OnPush(status, body);
                }
            }
            break;
        }
        case ix::WebSocketMessageType::Error:
            SPDLOG_WARN("SatellaClient: WebSocket error: {}", msg->errorInfo.reason);
            [[fallthrough]];
        case ix::WebSocketMessageType::Close: {
            std::lock_guard<std::mutex> lock(mMtx);
            mConnected = false;
            mResponseReady = true; // unblock any pending SendAndReceive
            mResponseValid = false;
            mCv.notify_all();
            break;
        }
        default:
            break;
    }
}

void Client::Connect(const std::string& url) {
    if (mUrl == url && mWs.getReadyState() == ix::ReadyState::Open) {
        return;
    }

    if (mWs.getReadyState() != ix::ReadyState::Closed) {
        mWs.stop();
    }

    mUrl = url;
    mWs.setUrl(url + "/ws");
    mWs.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) { OnMessage(msg); });

    {
        std::unique_lock<std::mutex> lock(mMtx);
        mConnected = false;
        mWs.start();
        mCv.wait_for(lock, std::chrono::seconds(10), [this] { return mConnected; });
    }

    if (!mConnected) {
        SPDLOG_WARN("SatellaClient: failed to connect to {}", url);
    } else {
        SPDLOG_INFO("SatellaClient: connected to {}", url);
    }
}

void Client::SendAndReceive(IPacket& packet) {
    if (mWs.getReadyState() != ix::ReadyState::Open) {
        SPDLOG_WARN("SatellaClient: not connected, skipping '{}'", packet.GetRoute());
        return;
    }

    auto reqData = buildRequest(packet.GetRoute());
    std::string payload(reqData.begin(), reqData.end());

    {
        std::unique_lock<std::mutex> lock(mMtx);
        mResponseReady      = false;
        mResponseValid      = false;
        mWaitingForResponse = true;
    }

    mWs.sendBinary(payload);

    std::unique_lock<std::mutex> lock(mMtx);
    mCv.wait_for(lock, std::chrono::seconds(10), [this] { return mResponseReady; });
    mWaitingForResponse = false;

    if (!mResponseValid) {
        SPDLOG_WARN("SatellaClient: no valid response for '{}'", packet.GetRoute());
        return;
    }

    packet.OnResponse(mResponseStatus, mResponseBody);
}

void Client::Execute(const std::string& url) {
    if (mPackets.empty()) {
        return;
    }

    ix::initNetSystem();
    mPhase = Phase::Connecting;
    Connect(url);

    if (!mConnected) {
        mPhase = Phase::Done;
        return;
    }

    mPhase = Phase::FetchingKeys;
    for (auto& packet : mPackets) {
        if (packet->IsBlocking()) {
            SendAndReceive(*packet);
        }
    }

    mPhase = Phase::Done;

    for (auto& packet : mPackets) {
        if (!packet->IsBlocking()) {
            SendAndReceive(*packet);
        }
    }

    for (auto& packet : mPackets) {
        if (packet->IsSubscription()) {
            mSubscriptions.push_back(packet.get());
        }
    }

    mPhase = Phase::Done;
}

// ---------------------------------------------------------------------------
// Built-in packets
// ---------------------------------------------------------------------------

class PublicKeysPacket : public IPacket {
  public:
    const char* GetRoute() const override {
        return "/v1/security/pub-keys";
    }
    bool IsBlocking() const override {
        return true;
    }

    void OnResponse(int16_t status, const std::string& body) override {
        if (status != 200) {
            SPDLOG_WARN("SatellaClient: pub-keys returned status {}", status);
            return;
        }

        nlohmann::json keys;
        try {
            keys = nlohmann::json::parse(body);
        } catch (const std::exception& e) {
            SPDLOG_WARN("SatellaClient: failed to parse pub-keys JSON: {}", e.what());
            return;
        }

        if (!keys.is_object()) {
            SPDLOG_WARN("SatellaClient: pub-keys response is not a JSON object");
            return;
        }

        auto keystore = Ship::Context::GetInstance()->GetKeystore();
        if (!keystore) {
            SPDLOG_WARN("SatellaClient: keystore not initialised yet");
            return;
        }

        int registered = 0;
        for (const auto& [name, value] : keys.items()) {
            if (!value.is_string()) {
                SPDLOG_WARN("SatellaClient: skipping non-string key '{}'", name);
                continue;
            }
            std::vector<uint8_t> keyData = StringHelper::HexToBytes(value.get<std::string>());
            if (keyData.empty()) {
                SPDLOG_WARN("SatellaClient: key '{}' decoded to empty bytes, skipping", name);
                continue;
            }
            if (keystore->HasKey(keyData)) {
                SPDLOG_DEBUG("SatellaClient: key '{}' already in keystore, skipping", name);
                continue;
            }
            keystore->AddKey(name, keyData, Ship::KeyOrigin::Game);
            SPDLOG_INFO("SatellaClient: registered public key '{}' from Satella", name);
            ++registered;
        }

        SPDLOG_INFO("SatellaClient: {} new key(s) registered from Satella", registered);
    }
};

SATELLA_REGISTER_PACKET(PublicKeysPacket);

// ---------------------------------------------------------------------------

class NotificationsPacket : public IPacket {
  public:
    const char* GetRoute() const override {
        return "/v1/notifications/subscribe";
    }
    bool IsSubscription() const override {
        return true;
    }
    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("SatellaClient: notifications subscribe returned status {}", status);
            return;
        }
        SPDLOG_INFO("SatellaClient: subscribed to notifications");
    }
    void OnPush(int16_t status, const std::string& body) override {
        if (status != 200 || body.empty()) return;

        nlohmann::json data;
        try {
            data = nlohmann::json::parse(body);
        } catch (const std::exception& e) {
            SPDLOG_WARN("SatellaClient: failed to parse notification push: {}", e.what());
            return;
        }

        Notification::Options opts;
        if (data.contains("prefix") && data["prefix"].is_string())
            opts.prefix = data["prefix"].get<std::string>();
        if (data.contains("message") && data["message"].is_string())
            opts.message = data["message"].get<std::string>();
        if (data.contains("suffix") && data["suffix"].is_string())
            opts.suffix = data["suffix"].get<std::string>();
        if (data.contains("duration") && data["duration"].is_number())
            opts.remainingTime = data["duration"].get<float>();

        Notification::Emit(opts);
    }
};

SATELLA_REGISTER_PACKET(NotificationsPacket);

} // namespace Satella
#endif