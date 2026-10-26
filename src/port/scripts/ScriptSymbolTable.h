#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name;
    const void* value;
} ScriptPointerSymbol;

typedef struct {
    const char* name;
    int64_t value;
} ScriptConstantSymbol;

// Behaviors, the functions behaviors and level scripts call, and the level scripts others jump to.
extern const ScriptPointerSymbol gScriptPointerSymbols[];
extern const size_t gScriptPointerSymbolCount;

// Integer constants: models, object fields and flags, object lists, interactions, sequences, dialogs, levels...
extern const ScriptConstantSymbol gScriptConstantSymbols[];
extern const size_t gScriptConstantSymbolCount;

#ifdef __cplusplus
}
#endif
