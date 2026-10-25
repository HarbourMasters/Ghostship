#pragma once
#ifdef USE_NETWORKING

#include <cstdint>
#include <functional>
#include <string>

// Channels mods send binary messages on through Satella. Each channel needs the player's permission.
namespace Relay {

enum class Permission { Pending, Allowed, Denied };

// Runs on the game thread for every message that arrives on the channel, with the sender's key in hex.
using MessageCallback =
    std::function<void(const std::string& channelId, const std::string& senderHex, const char* data, uint32_t size)>;

struct ChannelDef {
    std::string modName;     // shown in the permission prompt
    std::string description; // shown under the mod name
    std::string modVersion;  // keeps players on different versions of the mod apart, empty for none
    std::string owner;       // mod whose code onMessage calls, empty for the game
    MessageCallback onMessage;
};

// Registers a channel. A channel that is already registered keeps its definition, unless the mod that registered it
// has unloaded since.
void RegisterChannel(const std::string& channelId, ChannelDef def);

// Sends data on the channel. FALSE when the player hasn't allowed it or Satella isn't connected.
bool Send(const std::string& channelId, const char* data, uint32_t size);

Permission GetPermission(const std::string& channelId);

// Asks the player for the channel when they haven't answered yet.
void RequestPermission(const std::string& channelId);

} // namespace Relay
#endif
