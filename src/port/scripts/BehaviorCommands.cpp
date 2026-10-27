#include <cstring>
#include <string_view>

#include "types.h"
#include "behavior_macros.h"

#include "ScriptCommands.h"

#define V(i) ((uintptr_t)args[i])
#define SCRIPT_COMMAND(name, argc, call)                                                               \
    static int Emit_##name(const uintptr_t* args, uintptr_t* out) {                                    \
        (void)args;                                                                                    \
        const BehaviorScript cmd[] = { name call };                                                    \
        static_assert(sizeof(cmd) / sizeof(cmd[0]) <= SCRIPT_COMMAND_MAX_WORDS, #name " is too long"); \
        std::memcpy(out, cmd, sizeof(cmd));                                                            \
        return (int)(sizeof(cmd) / sizeof(cmd[0]));                                                    \
    }
#include "generated/BehaviorCommands.inc"
#undef SCRIPT_COMMAND

namespace {
struct BehaviorCommand {
    std::string_view name;
    int argc;
    int (*emit)(const uintptr_t* args, uintptr_t* out);
};

#define SCRIPT_COMMAND(name, argc, call) { #name, argc, Emit_##name },
const BehaviorCommand kCommands[] = {
#include "generated/BehaviorCommands.inc"
};
#undef SCRIPT_COMMAND

const BehaviorCommand* Find(const char* name) {
    for (const BehaviorCommand& cmd : kCommands) {
        if (cmd.name == name) {
            return &cmd;
        }
    }
    return nullptr;
}
} // namespace

extern "C" int ScriptCommands_EmitBehavior(const char* name, const uintptr_t* args, uintptr_t* out) {
    const BehaviorCommand* cmd = Find(name);
    return cmd != nullptr ? cmd->emit(args, out) : -1;
}

extern "C" int ScriptCommands_BehaviorArgCount(const char* name) {
    const BehaviorCommand* cmd = Find(name);
    return cmd != nullptr ? cmd->argc : -1;
}
