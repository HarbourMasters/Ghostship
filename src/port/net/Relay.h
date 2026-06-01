#pragma once
#ifdef USE_NETWORKING

#include <cstdint>
#include <functional>
#include <string>

namespace Relay {

enum class Permission { Pending, Allowed, Denied };

// Called on every inbound message for the channel.
using MessageCallback = std::function<void(const std::string& channelId,
                                           const char* data, uint32_t size)>;

struct ChannelDef {
    std::string     modName;     // shown in the permission prompt
    std::string     description; // shown below the mod name
    std::string     modVersion;  // optional: isolates channel by mod version (e.g. "1.2.0")
    MessageCallback onMessage;
};

// Register a channel before or after engine init. Duplicate ids are ignored.
void RegisterChannel(const std::string& channelId, ChannelDef def);

// Send binary data on a channel. Returns false if not permitted / not connected.
bool Send(const std::string& channelId, const char* data, uint32_t size);

Permission GetPermission(const std::string& channelId);

// Show the permission modal for `channelId` if still Pending.
void RequestPermission(const std::string& channelId);

// Called by RegisterShipInitFunc — activates already-permitted channels and
// queues permission prompts for pending ones.
void Init();

} // namespace Relay
#endif
