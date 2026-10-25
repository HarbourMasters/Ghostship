#ifdef ENABLE_SCRIPTING
#include "port/api/room.h"
#include "port/api/OnlineApi.hpp"
#include "port/api/SizedDef.hpp"

#ifdef USE_NETWORKING
#include "port/events/Events.h"
#include "port/net/SatellaClient.h"
#include "port/ShipInit.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>

// Everything here runs on the game thread; the network threads only queue work for it.
namespace {

constexpr const char* kIdChars = "._-";

struct Group {
    C_RoomGroupDef def;
    std::string modName;
    std::string description;
    std::string modVersion;
    OnlineApi::Caller caller;
};

struct Room {
    std::string group;
    std::string name;
    std::set<std::string> members;
};

std::map<std::string, Group> sGroups;
// By the room id on the wire.
std::map<std::string, Room> sRooms;

std::string PermKey(const std::string& group) {
    return "rooms." + group;
}

// group.room@gameVersion[-modVersion], unique per game and mod version.
std::string WireId(const std::string& group, const std::string& room) {
    std::string id = group + "." + room + "@" GHOSTSHIP_VERSION;
    const auto it = sGroups.find(group);
    if (it != sGroups.end() && !it->second.modVersion.empty()) {
        id += "-" + it->second.modVersion;
    }
    return id;
}

bool Allowed(const std::string& group) {
    return sGroups.contains(group) && Permissions::Get(PermKey(group)) == Permissions::State::Allowed;
}

void NotifyMember(const Room& room, const std::string& key, bool joined) {
    const auto it = sGroups.find(room.group);
    if (it == sGroups.end() || it->second.def.on_member == nullptr) {
        return;
    }
    const auto callback = it->second.def.on_member;
    it->second.caller.Call([&] { callback(room.group.c_str(), room.name.c_str(), key.c_str(), joined); });
}

// Takes the member list the server sent on joining, telling the mod who arrived or left since it last knew.
void SetMembers(const std::string& wireId, std::set<std::string> members) {
    auto it = sRooms.find(wireId);
    if (it == sRooms.end()) {
        return;
    }
    Room& room = it->second;
    const std::set<std::string> before = room.members;
    room.members = members;
    for (const auto& key : before) {
        if (!members.contains(key)) {
            NotifyMember(room, key, false);
        }
    }
    for (const auto& key : members) {
        if (!before.contains(key)) {
            NotifyMember(room, key, true);
        }
    }
}

void SendJoin(const std::string& wireId) {
    auto& client = Satella::Client::Instance();
    if (!client.IsConnected()) {
        return;
    }
    client.RegisterLive(std::make_unique<Satella::RequestPacket>(
        "/v1/room/" + wireId + "/join", std::vector<uint8_t>{}, [wireId](int16_t status, const std::string& body) {
            if (status != 200) {
                SPDLOG_WARN("Rooms: joining '{}' returned status {}", wireId, status);
                return;
            }
            const auto json = nlohmann::json::parse(body, nullptr, false);
            std::set<std::string> members;
            if (json.is_object() && json.contains("members") && json["members"].is_array()) {
                for (const auto& key : json["members"]) {
                    if (key.is_string()) {
                        members.insert(key.get<std::string>());
                    }
                }
            }
            Satella::Client::Instance().RunOnGameThread(
                [wireId, members = std::move(members)]() mutable { SetMembers(wireId, std::move(members)); });
        }));
}

void OnPush(const Satella::Push& push) {
    const auto it = sRooms.find(push.name);
    if (it == sRooms.end()) {
        return;
    }
    Room& room = it->second;
    switch (push.kind) {
        case Satella::PushKind::RoomJoin:
            if (room.members.insert(push.senderHex).second) {
                NotifyMember(room, push.senderHex, true);
            }
            break;
        case Satella::PushKind::RoomLeave:
            if (room.members.erase(push.senderHex) > 0) {
                NotifyMember(room, push.senderHex, false);
            }
            break;
        case Satella::PushKind::RoomMessage:
        case Satella::PushKind::RoomDirect: {
            const auto group = sGroups.find(room.group);
            if (group == sGroups.end() || group->second.def.on_message == nullptr) {
                break;
            }
            const auto callback = group->second.def.on_message;
            const int direct = push.kind == Satella::PushKind::RoomDirect;
            group->second.caller.Call([&] {
                callback(room.group.c_str(), room.name.c_str(), push.senderHex.c_str(), push.payload.data(),
                         static_cast<uint32_t>(push.payload.size()), direct);
            });
            break;
        }
        default:
            break;
    }
}

// The socket is gone, and with it every room: tell the mods everyone left, so the joins after reconnecting add up.
void DropAllMembers() {
    for (auto& [wireId, room] : sRooms) {
        const std::set<std::string> members = std::move(room.members);
        room.members.clear();
        for (const auto& key : members) {
            NotifyMember(room, key, false);
        }
    }
}

void RejoinAll() {
    for (const auto& [wireId, room] : sRooms) {
        SendJoin(wireId);
    }
}

void LeaveWire(const std::string& wireId) {
    auto it = sRooms.find(wireId);
    if (it == sRooms.end()) {
        return;
    }
    sRooms.erase(it);
    if (Satella::Client::Instance().IsConnected()) {
        Satella::Client::Instance().RegisterLive(
            std::make_unique<Satella::RequestPacket>("/v1/room/" + wireId + "/leave"));
    }
}

void ReleaseMod(const std::string& mod) {
    std::vector<std::string> groups;
    for (const auto& [id, group] : sGroups) {
        if (group.caller.mod == mod) {
            groups.push_back(id);
        }
    }
    for (const auto& id : groups) {
        std::vector<std::string> wireIds;
        for (const auto& [wireId, room] : sRooms) {
            if (room.group == id) {
                wireIds.push_back(wireId);
            }
        }
        for (const auto& wireId : wireIds) {
            LeaveWire(wireId);
        }
        sGroups.erase(id);
    }
}

bool ValidNames(const char* group, const char* room, const char* caller) {
    if (!OnlineApi::IsValidId(group, C_ROOM_GROUP_MAX, kIdChars) ||
        !OnlineApi::IsValidId(room, C_ROOM_NAME_MAX, kIdChars)) {
        SPDLOG_WARN("{}: invalid group or room name", caller);
        return false;
    }
    return true;
}

Room* FindJoined(const char* group, const char* room) {
    if (!OnlineApi::IsValidId(group, C_ROOM_GROUP_MAX, kIdChars) ||
        !OnlineApi::IsValidId(room, C_ROOM_NAME_MAX, kIdChars)) {
        return nullptr;
    }
    const auto it = sRooms.find(WireId(group, room));
    return it != sRooms.end() ? &it->second : nullptr;
}

} // namespace

extern "C" void C_RegisterRoomGroupSized(const char* group, const C_RoomGroupDef* def, uint32_t size) {
    const SizedDef<C_RoomGroupDef> sized(def, size);
    if (sized.get() == nullptr || !OnlineApi::IsValidId(group, C_ROOM_GROUP_MAX, kIdChars)) {
        SPDLOG_WARN("C_RegisterRoomGroup: invalid group id or definition");
        return;
    }
    if (sized.get()->on_message == nullptr) {
        SPDLOG_WARN("C_RegisterRoomGroup: '{}' has no on_message", group);
        return;
    }
    Group entry;
    entry.def = *sized.get();
    entry.modName = OnlineApi::Clamp(entry.def.mod_name, 64);
    entry.description = OnlineApi::Clamp(entry.def.description, 256);
    entry.modVersion = OnlineApi::Clamp(entry.def.mod_version, 32);
    entry.def.mod_name = nullptr;
    entry.def.description = nullptr;
    entry.def.mod_version = nullptr;
    Permissions::Register(PermKey(group),
                          { entry.modName + ": online rooms",
                            entry.modName + " wants to put you in rooms with other players, in\n\n    " + group +
                                "\n\n" + entry.description +
                                "\n\nAllow this mod to send and receive data with players in its "
                                "rooms over the internet?" });
    sGroups[group] = std::move(entry);
}

extern "C" C_PermissionState C_RoomGroupGetPermission(const char* group) {
    if (!OnlineApi::IsValidId(group, C_ROOM_GROUP_MAX, kIdChars)) {
        return C_PERM_DENIED;
    }
    return OnlineApi::ToC(Permissions::Get(PermKey(group)));
}

extern "C" void C_RoomGroupRequestPermission(const char* group) {
    if (OnlineApi::IsValidId(group, C_ROOM_GROUP_MAX, kIdChars)) {
        Permissions::Request(PermKey(group));
    }
}

extern "C" int C_JoinRoom(const char* group, const char* room) {
    if (!ValidNames(group, room, "C_JoinRoom") || !Allowed(group)) {
        return 0;
    }
    const std::string wireId = WireId(group, room);
    if (sRooms.contains(wireId)) {
        return 1;
    }
    sRooms[wireId] = { group, room, {} };
    SendJoin(wireId);
    return 1;
}

extern "C" int C_LeaveRoom(const char* group, const char* room) {
    if (FindJoined(group, room) == nullptr) {
        return 0;
    }
    LeaveWire(WireId(group, room));
    return 1;
}

extern "C" int C_RoomSend(const char* group, const char* room, const char* data, uint32_t size) {
    if (FindJoined(group, room) == nullptr || !Satella::Client::Instance().IsConnected() || data == nullptr ||
        size == 0 || size > C_ROOM_SEND_MAX) {
        return 0;
    }
    Satella::Client::Instance().SendRaw("/v1/room/" + WireId(group, room) + "/send", data, size);
    return 1;
}

extern "C" int C_RoomSendTo(const char* group, const char* room, const char* member_key_hex, const char* data,
                            uint32_t size) {
    if (FindJoined(group, room) == nullptr || !Satella::Client::Instance().IsConnected() || data == nullptr ||
        size == 0 || size > C_ROOM_SEND_MAX || member_key_hex == nullptr ||
        !OnlineApi::IsValidId(member_key_hex, C_PLAYER_KEY_HEX_LEN, "") ||
        strlen(member_key_hex) != C_PLAYER_KEY_HEX_LEN) {
        return 0;
    }
    Satella::Client::Instance().SendRaw("/v1/room/" + WireId(group, room) + "/send/" + member_key_hex, data, size);
    return 1;
}

extern "C" int C_RoomMembers(const char* group, const char* room, char (*keys)[C_PLAYER_KEY_HEX_LEN + 1],
                             int capacity) {
    const Room* joined = FindJoined(group, room);
    if (joined == nullptr) {
        return 0;
    }
    int i = 0;
    for (const auto& key : joined->members) {
        if (keys != nullptr && i < capacity) {
            snprintf(keys[i], C_PLAYER_KEY_HEX_LEN + 1, "%s", key.c_str());
        }
        i++;
    }
    return i;
}

static RegisterShipInitFunc sRoomsInit([] {
    ModOwner::AddReleaser(ReleaseMod);
    auto& client = Satella::Client::Instance();
    client.AddPushHandler([](const Satella::Push& push) {
        if (push.kind >= Satella::PushKind::RoomMessage && push.kind <= Satella::PushKind::RoomDirect) {
            Satella::Client::Instance().RunOnGameThread([push] { OnPush(push); });
        }
    });
    // At startup and after every reconnect: the new socket is in no room.
    REGISTER_LISTENER(SatellaConnected, EVENT_PRIORITY_NORMAL, [](IEvent*) { RejoinAll(); });
    REGISTER_LISTENER(SatellaDisconnected, EVENT_PRIORITY_NORMAL, [](IEvent*) { DropAllMembers(); });
});

#else

extern "C" void C_RegisterRoomGroupSized(const char*, const C_RoomGroupDef*, uint32_t) {
}
extern "C" C_PermissionState C_RoomGroupGetPermission(const char*) {
    return C_PERM_DENIED;
}
extern "C" void C_RoomGroupRequestPermission(const char*) {
}
extern "C" int C_JoinRoom(const char*, const char*) {
    return 0;
}
extern "C" int C_LeaveRoom(const char*, const char*) {
    return 0;
}
extern "C" int C_RoomSend(const char*, const char*, const char*, uint32_t) {
    return 0;
}
extern "C" int C_RoomSendTo(const char*, const char*, const char*, const char*, uint32_t) {
    return 0;
}
extern "C" int C_RoomMembers(const char*, const char*, char (*)[C_PLAYER_KEY_HEX_LEN + 1], int) {
    return 0;
}

#endif
#endif
