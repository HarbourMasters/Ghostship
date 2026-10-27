#pragma once

#include <stdint.h>

#include "api.h"
#include "port/api/online.h"
#include "port/api/permissions.h"

// Rooms of players, through Satella: everyone in a room gets the messages sent to it, a message can go to one member,
// and members hear when others join or leave, at once when someone disconnects. Rooms come in groups, such as one
// room per town, and the player is asked once per group. Rooms are joined again after a reconnect. Only with
// Satella; elsewhere joining fails.

#define C_ROOM_GROUP_MAX 64
#define C_ROOM_NAME_MAX 48
#define C_ROOM_SEND_MAX (64 * 1024)
#define C_ROOM_MEMBERS_MAX 64

// Runs on the game thread for every message in a room: sent to the whole room, or to this player alone when direct is
// TRUE.
typedef void (*C_RoomMessageCallback)(const char* group, const char* room, const char* sender_key_hex, const char* data,
                                      uint32_t size, int direct);
// Runs on the game thread when a player joins (joined TRUE) or leaves a room this player is in, including the players
// already there when this player joins. When the connection drops, everyone leaves; after reconnecting they join
// again.
typedef void (*C_RoomMemberCallback)(const char* group, const char* room, const char* member_key_hex, int joined);

typedef struct C_RoomGroupDef_s {
    const char* mod_name;    // shown in the permission prompt
    const char* description; // one sentence telling the player what the rooms are for
    C_RoomMessageCallback on_message;
    C_RoomMemberCallback on_member; // NULL to not be told
    const char* mod_version;        // keeps players on different versions of the mod apart, NULL for none
} C_RoomGroupDef;

// Registers a group of rooms, e.g. "mymod.towns". Group ids are letters, digits, '.', '_' and '-', up to
// C_ROOM_GROUP_MAX characters; room names the same, up to C_ROOM_NAME_MAX.
#define C_RegisterRoomGroup(group, def) C_RegisterRoomGroupSized((group), (def), sizeof(*(def)))
GS_API void C_RegisterRoomGroupSized(const char* group, const C_RoomGroupDef* def, uint32_t size);
GS_API C_PermissionState C_RoomGroupGetPermission(const char* group);
// Asks the player for the group when they haven't answered yet.
GS_API void C_RoomGroupRequestPermission(const char* group);

// Joins a room of the group. Offline, it is joined once Satella connects. Returns FALSE when the group isn't
// allowed or the names aren't valid. Rooms hold up to C_ROOM_MEMBERS_MAX players.
GS_API int C_JoinRoom(const char* group, const char* room);
GS_API int C_LeaveRoom(const char* group, const char* room);
// Sends up to C_ROOM_SEND_MAX bytes to everyone else in the room, up to 60 messages a second. Returns FALSE when
// not in the room or offline.
GS_API int C_RoomSend(const char* group, const char* room, const char* data, uint32_t size);
// Sends to one member only.
GS_API int C_RoomSendTo(const char* group, const char* room, const char* member_key_hex, const char* data,
                        uint32_t size);
// Writes the keys of the other members, up to capacity, and returns how many there are.
GS_API int C_RoomMembers(const char* group, const char* room, char (*keys)[C_PLAYER_KEY_HEX_LEN + 1], int capacity);
