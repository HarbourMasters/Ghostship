#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The most words a single level or behavior command macro expands to.
#define SCRIPT_COMMAND_MAX_WORDS 16

// Expands level command macro `name` into `out`. Returns the words written, or -1 for an unknown command.
int ScriptCommands_EmitLevel(const char* name, const uintptr_t* args, uintptr_t* out);
// Argument count, or -1 for an unknown command.
int ScriptCommands_LevelArgCount(const char* name);

// Behavior script commands.
int ScriptCommands_EmitBehavior(const char* name, const uintptr_t* args, uintptr_t* out);
int ScriptCommands_BehaviorArgCount(const char* name);

#ifdef __cplusplus
}
#endif
