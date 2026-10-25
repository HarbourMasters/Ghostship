#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "api.h"

// Settings of a mod go under "gMods.<mod>.", so they don't clash with the game's or other mods' settings. The Mods
// menu keeps its own under "gModList.".
#define C_MOD_CVAR(mod, name) "gMods." mod "." name

GS_API int32_t CVarGetInteger(const char* name, int32_t defaultValue);
GS_API float CVarGetFloat(const char* name, float defaultValue);
GS_API const char* CVarGetString(const char* name, const char* defaultValue);
GS_API void CVarSetInteger(const char* name, int32_t value);
GS_API void CVarSetFloat(const char* name, float value);
GS_API void CVarSetString(const char* name, const char* value);
GS_API void CVarRegisterInteger(const char* name, int32_t defaultValue);
GS_API void CVarRegisterFloat(const char* name, float defaultValue);
GS_API void CVarRegisterString(const char* name, const char* defaultValue);
GS_API void CVarClear(const char* name);
GS_API bool CVarExists(const char* name);
GS_API void CVarSave(void);
