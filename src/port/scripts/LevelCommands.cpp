#include <cstring>
#include <string_view>

#include "types.h"
#include "level_commands.h"

#include "ScriptCommands.h"

#define V(i) ((uintptr_t)args[i])
#define SCRIPT_COMMAND(name, argc, call)                                   \
    static int Emit_##name(const uintptr_t* args, uintptr_t* out) {        \
        (void)args;                                                        \
        const LevelScript cmd[] = { name call };                           \
        static_assert(sizeof(cmd) / sizeof(cmd[0]) <= SCRIPT_COMMAND_MAX_WORDS, #name " is too long"); \
        std::memcpy(out, cmd, sizeof(cmd));                                \
        return (int)(sizeof(cmd) / sizeof(cmd[0]));                        \
    }
#include "generated/LevelCommands.inc"
#undef SCRIPT_COMMAND

namespace {
struct LevelCommand {
    std::string_view name;
    int argc;
    int (*emit)(const uintptr_t* args, uintptr_t* out);
};

#define SCRIPT_COMMAND(name, argc, call) { #name, argc, Emit_##name },
const LevelCommand kCommands[] = {
#include "generated/LevelCommands.inc"
};
#undef SCRIPT_COMMAND

const LevelCommand* Find(const char* name) {
    for (const LevelCommand& cmd : kCommands) {
        if (cmd.name == name) {
            return &cmd;
        }
    }
    return nullptr;
}
} // namespace

extern "C" int ScriptCommands_EmitLevel(const char* name, const uintptr_t* args, uintptr_t* out) {
    const LevelCommand* cmd = Find(name);
    return cmd != nullptr ? cmd->emit(args, out) : -1;
}

extern "C" int ScriptCommands_LevelArgCount(const char* name) {
    const LevelCommand* cmd = Find(name);
    return cmd != nullptr ? cmd->argc : -1;
}
