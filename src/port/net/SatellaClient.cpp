#ifdef USE_NETWORKING
#include "port/net/SatellaClient.h"
#include "port/net/PlayerIdentity.h"
#include "port/ShipCompat.h"
#include "port/ShipInit.hpp"
#include "port/ui/Notification.h"

#include <ship/config/Config.h>
#include <ship/security/Keystore.h>
#include <ship/utils/StringHelper.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include "port/events/Events.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

#if !defined(_WIN32)
#include <unistd.h>
#endif

static ix::WebSocket sWs;

namespace Satella {

static constexpr uint8_t kFrameJson = 0x02;
static constexpr uint8_t kFrameRaw = 0x03;
static constexpr uint8_t kFramePush = 0x04;
// Signed with the nonce from /v1/player/challenge, so the signature can't be reused as anything else.
static constexpr const char* kAuthTag = "satella-player-auth-v1:";

static void WriteU32LE(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

static int16_t ReadI16LE(const uint8_t* buf) {
    return static_cast<int16_t>(static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8));
}

// "HM64", the frame type, then the route with its length.
static std::vector<uint8_t> BuildFrame(uint8_t type, const std::string& route) {
    std::vector<uint8_t> frame = { 'H', 'M', '6', '4', type };
    WriteU32LE(frame, static_cast<uint32_t>(route.size()));
    frame.insert(frame.end(), route.begin(), route.end());
    return frame;
}

// kind | u16 name length | name | 32-byte sender | payload, after the 5-byte header.
static bool ParsePush(const uint8_t* data, size_t size, Push& push) {
    if (size < 3) {
        return false;
    }
    const size_t nameLen = static_cast<size_t>(data[1]) | (static_cast<size_t>(data[2]) << 8);
    if (size < 3 + nameLen + 32) {
        return false;
    }
    push.kind = static_cast<PushKind>(data[0]);
    push.name.assign(reinterpret_cast<const char*>(data + 3), nameLen);
    const uint8_t* sender = data + 3 + nameLen;
    const bool anonymous = std::all_of(sender, sender + 32, [](uint8_t b) { return b == 0; });
    push.senderHex = anonymous ? std::string() : StringHelper::BytesToHex({ sender, sender + 32 });
    push.payload.assign(reinterpret_cast<const char*>(sender + 32), size - 3 - nameLen - 32);
    return true;
}

Client& Client::Instance() {
    static Client instance;
    return instance;
}

Client::~Client() {
    {
        std::lock_guard<std::mutex> lock(mQueueMtx);
        mStopping = true;
    }
    mQueueCv.notify_all();
    if (mWorker.joinable()) {
        mWorker.join();
    }
    if (sWs.getReadyState() != ix::ReadyState::Closed) {
        sWs.stop();
    }
    ix::uninitNetSystem();
}

void Client::Register(std::unique_ptr<IPacket> packet) {
    std::lock_guard<std::mutex> lock(mPacketsMtx);
    mPackets.push_back(std::move(packet));
}

bool Client::IsConnected() const {
    return sWs.getReadyState() == ix::ReadyState::Open;
}

void Client::Keep(const std::shared_ptr<IPacket>& packet) {
    if (!packet->IsSubscription()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mPacketsMtx);
    mSubscriptions.push_back(packet);
}

void Client::RegisterLive(std::unique_ptr<IPacket> packet) {
    if (!IsConnected()) {
        if (mPhase.load(std::memory_order_relaxed) == Phase::Idle) {
            Register(std::move(packet));
        } else if (packet->IsSubscription()) {
            Keep(std::shared_ptr<IPacket>(std::move(packet)));
        } else {
            SPDLOG_DEBUG("Satella: dropping '{}', not connected", packet->GetRoute());
        }
        return;
    }
    std::shared_ptr<IPacket> shared = std::move(packet);
    RunOnWorker([this, shared] {
        SendAndReceive(*shared);
        Keep(shared);
    });
}

void Client::RunOnWorker(std::function<void()> work) {
    {
        std::lock_guard<std::mutex> lock(mQueueMtx);
        mQueue.push_back(std::move(work));
        if (!mWorker.joinable()) {
            mWorker = std::thread([this] { WorkerLoop(); });
        }
    }
    mQueueCv.notify_one();
}

void Client::WorkerLoop() {
    while (true) {
        std::function<void()> work;
        {
            std::unique_lock<std::mutex> lock(mQueueMtx);
            mQueueCv.wait(lock, [this] { return mStopping || !mQueue.empty(); });
            if (mStopping) {
                return;
            }
            work = std::move(mQueue.front());
            mQueue.pop_front();
        }
        work();
    }
}

void Client::SendRaw(const std::string& route, const void* data, size_t size) {
    if (!IsConnected()) {
        return;
    }
    std::vector<uint8_t> frame = BuildFrame(kFrameRaw, route);
    const auto* bytes = static_cast<const uint8_t*>(data);
    frame.insert(frame.end(), bytes, bytes + size);
    sWs.sendBinary(std::string(frame.begin(), frame.end()));
}

void Client::AddPushHandler(std::function<void(const Push& push)> handler) {
    std::lock_guard<std::mutex> lock(mHandlersMtx);
    mPushHandlers.push_back(std::move(handler));
}

void Client::AddReconnectHandler(std::function<void()> handler) {
    std::lock_guard<std::mutex> lock(mHandlersMtx);
    mReconnectHandlers.push_back(std::move(handler));
}

void Client::RunOnGameThread(std::function<void()> work) {
    std::lock_guard<std::mutex> lock(mGameMtx);
    mGameWork.push_back(std::move(work));
}

void Client::Poll() {
    std::vector<std::function<void()>> work;
    {
        std::lock_guard<std::mutex> lock(mGameMtx);
        work.swap(mGameWork);
    }
    for (auto& fn : work) {
        fn();
    }
}

void Client::OnMessage(const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            bool reconnected;
            {
                std::lock_guard<std::mutex> lock(mMtx);
                mConnected = true;
                reconnected = mEverConnected;
                mEverConnected = true;
                mCv.notify_all();
            }
            if (reconnected) {
                SPDLOG_INFO("Satella: reconnected");
                RunOnWorker([this] { Resume(); });
            }
            break;
        }
        case ix::WebSocketMessageType::Message: {
            const auto* data = reinterpret_cast<const uint8_t*>(msg->str.data());
            const size_t size = msg->str.size();
            constexpr size_t kHeaderSize = 5;

            if (size < kHeaderSize || data[0] != 'H' || data[1] != 'M') {
                SPDLOG_WARN("Satella: malformed message ({} bytes)", size);
                break;
            }

            const uint8_t type = data[2];
            const int16_t status = ReadI16LE(data + 3);

            // Push frames are never a reply, so they can't be taken for the one a request waits for.
            if (type == kFramePush) {
                Push push;
                if (!ParsePush(data + kHeaderSize, size - kHeaderSize, push)) {
                    SPDLOG_WARN("Satella: malformed push ({} bytes)", size);
                    break;
                }
                std::vector<std::function<void(const Push&)>> handlers;
                {
                    std::lock_guard<std::mutex> lock(mHandlersMtx);
                    handlers = mPushHandlers;
                }
                for (const auto& handler : handlers) {
                    handler(push);
                }
                break;
            }

            const std::string body = (type != 0x01 && size > kHeaderSize)
                                         ? std::string(msg->str.begin() + kHeaderSize, msg->str.end())
                                         : std::string();

            bool isResponse;
            {
                std::lock_guard<std::mutex> lock(mMtx);
                isResponse = mWaitingForResponse;
                if (isResponse) {
                    mResponseStatus = status;
                    mResponseBody = body;
                    mResponseValid = true;
                    mResponseReady = true;
                    mWaitingForResponse = false;
                    mCv.notify_all();
                }
            }

            if (!isResponse) {
                std::vector<std::shared_ptr<IPacket>> subscriptions;
                {
                    std::lock_guard<std::mutex> lock(mPacketsMtx);
                    subscriptions = mSubscriptions;
                }
                for (const auto& sub : subscriptions) {
                    sub->OnPush(status, type, body);
                }
            }
            break;
        }
        case ix::WebSocketMessageType::Error:
            SPDLOG_WARN("Satella: WebSocket error: {}", msg->errorInfo.reason);
            [[fallthrough]];
        case ix::WebSocketMessageType::Close: {
            bool wasConnected;
            {
                std::lock_guard<std::mutex> lock(mMtx);
                wasConnected = mConnected;
                mConnected = false;
                mResponseReady = true;
                mResponseValid = false;
                mCv.notify_all();
            }
            mVerified = false;
            if (wasConnected) {
                RunOnGameThread([] { CALL_EVENT(SatellaDisconnected); });
            }
            break;
        }
        default:
            break;
    }
}

static ix::SocketTLSOptions BuildTLSOptions() {
    ix::SocketTLSOptions opts;
#if defined(__ANDROID__)
    // Android keeps its trusted CAs as one PEM file each, in the updatable Conscrypt module since Android 14. Passing
    // them in memory lets mbedtls trust the same certificates the system does.
    static const std::string bundle = [] {
        std::string pem;
        for (const char* dir : { "/apex/com.android.conscrypt/cacerts", "/system/etc/security/cacerts" }) {
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                std::ifstream in(entry.path(), std::ios::binary);
                pem.append(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
                pem += '\n';
            }
            if (!pem.empty()) {
                break;
            }
        }
        return pem;
    }();
    if (!bundle.empty()) {
        opts.caFile = bundle;
    } else {
        SPDLOG_WARN("Satella: no system CA certificates found");
    }
#elif defined(__linux__) || defined(__APPLE__)
    // ixwebsocket's mbedtls backend only finds the system certificates on Windows.
#if defined(__APPLE__)
    static constexpr const char* kCAPaths[] = {
        "/etc/ssl/cert.pem",
    };
#else
    static constexpr const char* kCAPaths[] = {
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
    };
#endif
    for (const char* path : kCAPaths) {
        if (access(path, R_OK) == 0) {
            opts.caFile = path;
            break;
        }
    }
#endif
    return opts;
}

void Client::Connect(const std::string& url) {
    if (mUrl == url && IsConnected()) {
        return;
    }
    if (sWs.getReadyState() != ix::ReadyState::Closed) {
        sWs.stop();
    }

    mUrl = url;
    sWs.setUrl(url + "/ws?v=" GHOSTSHIP_VERSION);
#if defined(__linux__) || defined(__APPLE__)
    sWs.setTLSOptions(BuildTLSOptions());
#endif
    sWs.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) { OnMessage(msg); });

    std::unique_lock<std::mutex> lock(mMtx);
    mConnected = false;
    sWs.start();
    mCv.wait_for(lock, std::chrono::seconds(10), [this] { return mConnected; });

    if (!mConnected) {
        SPDLOG_WARN("Satella: could not connect to {}", url);
    } else {
        SPDLOG_INFO("Satella: connected to {}", url);
    }
}

void Client::SendAndReceive(IPacket& packet) {
    std::lock_guard<std::mutex> request(mRequestMtx);
    if (!IsConnected()) {
        SPDLOG_WARN("Satella: not connected, skipping '{}'", packet.GetRoute());
        return;
    }

    const std::vector<uint8_t> body = packet.GetBody();
    std::vector<uint8_t> frame = BuildFrame(body.empty() ? kFrameJson : kFrameRaw, packet.GetRoute());
    frame.insert(frame.end(), body.begin(), body.end());
    {
        std::lock_guard<std::mutex> lock(mMtx);
        mResponseReady = false;
        mResponseValid = false;
        mWaitingForResponse = true;
    }

    sWs.sendBinary(std::string(frame.begin(), frame.end()));

    int16_t status;
    std::string reply;
    {
        std::unique_lock<std::mutex> lock(mMtx);
        mCv.wait_for(lock, std::chrono::seconds(10), [this] { return mResponseReady; });
        mWaitingForResponse = false;
        if (!mResponseValid) {
            SPDLOG_WARN("Satella: no response for '{}'", packet.GetRoute());
            return;
        }
        status = mResponseStatus;
        reply = std::move(mResponseBody);
    }

    packet.OnResponse(status, reply);
}

// Asks for a nonce, then proves the player key by signing it.
class ChallengePacket : public IPacket {
  public:
    std::vector<uint8_t> nonce;

    const char* GetRoute() const override {
        return "/v1/player/challenge";
    }

    void OnResponse(int16_t status, const std::string& body) override {
        const auto json = nlohmann::json::parse(body, nullptr, false);
        if (status == 200 && json.is_object() && json.value("nonce", std::string()).size() == 64) {
            nonce = StringHelper::HexToBytes(json["nonce"].get<std::string>());
        }
    }
};

class VerifyPacket : public IPacket {
  public:
    VerifyPacket(const std::string& pubKeyHex, const std::vector<uint8_t>& nonce)
        : mRoute("/v1/player/" + pubKeyHex + "/verify") {
        std::vector<uint8_t> message(kAuthTag, kAuthTag + strlen(kAuthTag));
        message.insert(message.end(), nonce.begin(), nonce.end());
        mSignature = PlayerIdentity::Sign(message.data(), message.size());
    }

    bool verified = false;

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    std::vector<uint8_t> GetBody() const override {
        return mSignature;
    }

    void OnResponse(int16_t status, const std::string&) override {
        verified = status == 200;
    }

  private:
    std::string mRoute;
    std::vector<uint8_t> mSignature;
};

void Client::Authenticate() {
    const auto pub = PlayerIdentity::GetPublicKey();
    if (pub.empty()) {
        return;
    }
    ChallengePacket challenge;
    SendAndReceive(challenge);
    if (challenge.nonce.size() != 32) {
        SPDLOG_WARN("Satella: the server didn't send a key challenge; pushes won't say who sent them");
        return;
    }
    VerifyPacket verify(StringHelper::BytesToHex(pub), challenge.nonce);
    SendAndReceive(verify);
    mVerified = verify.verified;
    if (!verify.verified) {
        SPDLOG_WARN("Satella: the server refused the player key");
    }
}

// After a reconnect: the new socket has no key and no subscriptions.
void Client::Resume() {
    Authenticate();
    std::vector<std::shared_ptr<IPacket>> subscriptions;
    std::vector<std::function<void()>> handlers;
    {
        std::lock_guard<std::mutex> lock(mPacketsMtx);
        subscriptions = mSubscriptions;
    }
    {
        std::lock_guard<std::mutex> lock(mHandlersMtx);
        handlers = mReconnectHandlers;
    }
    for (const auto& sub : subscriptions) {
        SendAndReceive(*sub);
    }
    for (const auto& handler : handlers) {
        handler();
    }
    RunOnGameThread([] { CALL_EVENT(SatellaConnected); });
}

void Client::Execute() {
    std::vector<std::shared_ptr<IPacket>> packets;
    {
        std::lock_guard<std::mutex> lock(mPacketsMtx);
        packets = mPackets;
    }
    if (packets.empty()) {
        return;
    }

    ix::initNetSystem();
    mPhase = Phase::Connecting;
    auto config = ShipCompat::GetConfig();
    Connect(config != nullptr ? config->GetString("Satella.Host", SATELLA_HOST) : SATELLA_HOST);

    if (!mConnected) {
        mPhase = Phase::Done;
        return;
    }

    mPhase = Phase::FetchingKeys;
    Authenticate();
    for (const auto& packet : packets) {
        if (packet->IsBlocking()) {
            SendAndReceive(*packet);
        }
    }

    mPhase = Phase::Done;
    for (const auto& packet : packets) {
        if (!packet->IsBlocking()) {
            SendAndReceive(*packet);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mPacketsMtx);
        for (const auto& packet : packets) {
            if (packet->IsSubscription()) {
                mSubscriptions.push_back(packet);
            }
        }
    }
    RunOnGameThread([] { CALL_EVENT(SatellaConnected); });
}

// Registers this install's player key, so the stores can check its signatures.
class RegisterPlayerPacket : public IPacket {
    mutable std::string mRoute;

  public:
    const char* GetRoute() const override {
        if (mRoute.empty()) {
            const auto pub = PlayerIdentity::GetPublicKey();
            mRoute = "/v1/player/" + StringHelper::BytesToHex({ pub.begin(), pub.end() }) + "/register";
        }
        return mRoute.c_str();
    }

    bool IsBlocking() const override {
        return true;
    }

    void OnResponse(int16_t status, const std::string&) override {
        if (status != 200) {
            SPDLOG_WARN("Satella: player registration returned status {}", status);
        } else {
            SPDLOG_INFO("Satella: player registered");
        }
    }
};

SATELLA_REGISTER_PACKET(RegisterPlayerPacket);

// Adds the keys Satella vouches for to the keystore, before the game mounts mods, so mods signed with them load
// without asking.
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
            SPDLOG_WARN("Satella: pub-keys returned status {}", status);
            return;
        }

        const auto keys = nlohmann::json::parse(body, nullptr, false);
        if (!keys.is_object()) {
            SPDLOG_WARN("Satella: pub-keys response is not a JSON object");
            return;
        }

        auto keystore = ShipCompat::GetKeystore();
        if (keystore == nullptr) {
            SPDLOG_WARN("Satella: keystore not initialised yet");
            return;
        }

        int added = 0;
        for (const auto& [name, value] : keys.items()) {
            if (!value.is_string()) {
                continue;
            }
            const std::vector<uint8_t> keyData = StringHelper::HexToBytes(value.get<std::string>());
            if (keyData.empty() || keystore->HasKey(keyData)) {
                continue;
            }
            keystore->AddKey(name, keyData, Ship::KeyOrigin::Game);
            ++added;
        }
        SPDLOG_INFO("Satella: {} new public key(s)", added);
    }
};

SATELLA_REGISTER_PACKET(PublicKeysPacket);

static void ShowNotification(const std::string& body) {
    const auto data = nlohmann::json::parse(body, nullptr, false);
    if (!data.is_object()) {
        return;
    }
    Notification::Options opts;
    opts.prefix = data.value("prefix", std::string());
    opts.message = data.value("message", std::string());
    opts.suffix = data.value("suffix", std::string());
    if (data.contains("duration") && data["duration"].is_number()) {
        opts.remainingTime = data["duration"].get<float>();
    }
    Client::Instance().RunOnGameThread([opts] { Notification::Emit(opts); });
}

// Shows the notifications Satella pushes. Servers without push frames send them as plain JSON.
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
            SPDLOG_WARN("Satella: notifications subscribe returned status {}", status);
        }
    }

    void OnPush(int16_t status, uint8_t packetType, const std::string& body) override {
        if (status == 200 && packetType == kFrameJson && !body.empty()) {
            ShowNotification(body);
        }
    }
};

SATELLA_REGISTER_PACKET(NotificationsPacket);

} // namespace Satella

static void SatellaPoll(IEvent*) {
    Satella::Client::Instance().Poll();
}

static RegisterShipInitFunc sSatellaInit([] {
    REGISTER_LISTENER(GameLoopTick, EVENT_PRIORITY_NORMAL, SatellaPoll);
    Satella::Client::Instance().AddPushHandler([](const Satella::Push& push) {
        if (push.kind == Satella::PushKind::Notification) {
            Satella::ShowNotification(push.payload);
        }
    });
});

#endif
