#pragma once

#ifndef __SWITCH__
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <ixwebsocket/IXWebSocket.h>

#define SATELLA_HOST "wss://satella.net64.dev"

namespace Satella {

class IPacket {
public:
    virtual ~IPacket() = default;
    virtual const char* GetRoute() const = 0;
    virtual void OnResponse(int16_t status, const std::string& body) = 0;
    virtual bool IsBlocking() const { return false; }
    virtual bool IsSubscription() const { return false; }
    virtual void OnPush(int16_t status, const std::string& body) {}
};

class Client {
public:
    static Client& Instance();

    void Register(std::unique_ptr<IPacket> packet);
    void Execute(const std::string& url = SATELLA_HOST);

private:
    Client() = default;
    ~Client();

    void Connect(const std::string& url);
    void SendAndReceive(IPacket& packet);
    void OnMessage(const ix::WebSocketMessagePtr& msg);

    std::vector<std::unique_ptr<IPacket>> mPackets;
    std::vector<IPacket*>                 mSubscriptions;

    ix::WebSocket mWs;
    std::string   mUrl;

    std::mutex              mMtx;
    std::condition_variable mCv;

    bool    mConnected          = false;
    bool    mWaitingForResponse = false;
    bool    mResponseReady      = false;
    bool    mResponseValid      = false;
    int16_t mResponseStatus     = 0;
    std::string mResponseBody;
};

template<typename T>
struct PacketAutoReg {
    PacketAutoReg() { Client::Instance().Register(std::make_unique<T>()); }
};

#define SATELLA_REGISTER_PACKET(T) \
    static ::Satella::PacketAutoReg<T> _satella_pkt_##T

} // namespace Satella
#endif