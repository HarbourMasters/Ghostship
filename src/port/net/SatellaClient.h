#pragma once

#ifdef USE_NETWORKING
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <ixwebsocket/IXWebSocketMessage.h>

// Overridden by "Satella.Host" in the config.
#define SATELLA_HOST "wss://satella.net64.dev"

namespace Satella {

enum class Phase { Idle, Connecting, FetchingKeys, Done };

// What a push frame carries. Satella sends them once the client has proven its player key.
enum class PushKind : uint8_t {
    Relay = 1,
    RoomMessage = 2,
    RoomJoin = 3,
    RoomLeave = 4,
    RoomDirect = 5,
    Notification = 6,
};

struct Push {
    PushKind kind;
    std::string name;      // relay channel or room, as sent on the wire
    std::string senderHex; // key of the sender in hex, empty when unknown
    std::string payload;
};

class IPacket {
  public:
    virtual ~IPacket() = default;
    virtual const char* GetRoute() const = 0;
    // Sent after the route; a request with a body goes as a RAW frame.
    virtual std::vector<uint8_t> GetBody() const {
        return {};
    }
    virtual void OnResponse(int16_t status, const std::string& body) = 0;
    // Sent before the game mounts its archives, e.g. the keys mods are signed with.
    virtual bool IsBlocking() const {
        return false;
    }
    // Sent again after a reconnect, and gets the pushes of servers that don't send push frames.
    virtual bool IsSubscription() const {
        return false;
    }
    virtual void OnPush(int16_t status, uint8_t packetType, const std::string& body) {
    }
};

// A one-off request: a route, an optional body and what to do with the reply (on the worker thread).
class RequestPacket : public IPacket {
  public:
    using ResponseFunc = std::function<void(int16_t status, const std::string& body)>;

    RequestPacket(std::string route, std::vector<uint8_t> body = {}, ResponseFunc onResponse = nullptr)
        : mRoute(std::move(route)), mBody(std::move(body)), mOnResponse(std::move(onResponse)) {
    }

    const char* GetRoute() const override {
        return mRoute.c_str();
    }

    std::vector<uint8_t> GetBody() const override {
        return mBody;
    }

    void OnResponse(int16_t status, const std::string& body) override {
        if (mOnResponse) {
            mOnResponse(status, body);
        }
    }

  private:
    std::string mRoute;
    std::vector<uint8_t> mBody;
    ResponseFunc mOnResponse;
};

class Client {
  public:
    static Client& Instance();

    // Adds a packet Execute sends once connected.
    void Register(std::unique_ptr<IPacket> packet);
    // Connects, proves the player key and sends the registered packets, the blocking ones first. Waits for every
    // response, so it runs off the game thread.
    void Execute();
    Phase GetPhase() const {
        return mPhase.load(std::memory_order_relaxed);
    }
    bool IsConnected() const;
    // The server accepted the player key on this connection, so pushes say who sent them.
    bool IsVerified() const {
        return mVerified.load(std::memory_order_relaxed);
    }

    // Sends a packet on the open connection from the worker thread, or with the next Execute when not connected.
    void RegisterLive(std::unique_ptr<IPacket> packet);
    // Runs work on the worker thread, after what was queued before it.
    void RunOnWorker(std::function<void()> work);
    // Sends a request and waits up to 10 seconds for its reply. Only from the worker thread or Execute.
    void SendAndReceive(IPacket& packet);

    // Sends a frame that gets no reply: the route and the body. Does nothing when not connected.
    void SendRaw(const std::string& route, const void* data, size_t size);

    // Called on the network thread for every push frame.
    void AddPushHandler(std::function<void(const Push& push)> handler);
    // Runs on the worker thread after a reconnect, once the key is proven and the subscriptions are sent again.
    void AddReconnectHandler(std::function<void()> handler);

    // Queues work for the game thread, run by Poll once per main loop iteration.
    void RunOnGameThread(std::function<void()> work);
    void Poll();

  private:
    Client() = default;
    ~Client();

    void Connect(const std::string& url);
    void Authenticate();
    void Resume();
    void OnMessage(const ix::WebSocketMessagePtr& msg);
    void Keep(const std::shared_ptr<IPacket>& packet);
    void WorkerLoop();

    std::mutex mPacketsMtx;
    std::vector<std::shared_ptr<IPacket>> mPackets;
    std::vector<std::shared_ptr<IPacket>> mSubscriptions;

    std::string mUrl;

    std::mutex mRequestMtx;
    std::mutex mMtx;
    std::condition_variable mCv;
    std::atomic<Phase> mPhase{ Phase::Idle };
    std::atomic<bool> mVerified{ false };
    bool mConnected = false;
    bool mEverConnected = false;
    bool mWaitingForResponse = false;
    bool mResponseReady = false;
    bool mResponseValid = false;
    int16_t mResponseStatus = 0;
    std::string mResponseBody;

    std::mutex mQueueMtx;
    std::condition_variable mQueueCv;
    std::deque<std::function<void()>> mQueue;
    std::thread mWorker;
    bool mStopping = false;

    std::mutex mHandlersMtx;
    std::vector<std::function<void(const Push&)>> mPushHandlers;
    std::vector<std::function<void()>> mReconnectHandlers;

    std::mutex mGameMtx;
    std::vector<std::function<void()>> mGameWork;
};

template <typename T> struct PacketAutoReg {
    PacketAutoReg() {
        Client::Instance().Register(std::make_unique<T>());
    }
};

#define SATELLA_REGISTER_PACKET(T) static ::Satella::PacketAutoReg<T> _satella_pkt_##T

} // namespace Satella
#endif
