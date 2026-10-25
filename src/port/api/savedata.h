#pragma once

#include <stdint.h>

#include "api.h"

// Values kept in the save file being played, under keys like "mymod.visits". They follow the game's own save data:
// read with the save files at startup, written whenever the game saves that file, erased and copied with it in the
// file select, and taken back to the last save after a game over. A new file starts with none. Read them from
// OnGameFileLoad or later; before a file is picked they belong to file 1.

GS_API int C_SaveDataHas(const char* key);
GS_API void C_SaveDataRemove(const char* key);

GS_API int32_t C_SaveDataGetInt(const char* key, int32_t defaultValue);
GS_API void C_SaveDataSetInt(const char* key, int32_t value);
GS_API float C_SaveDataGetFloat(const char* key, float defaultValue);
GS_API void C_SaveDataSetFloat(const char* key, float value);
// The string stays valid until the key is set again or another file is played.
GS_API const char* C_SaveDataGetString(const char* key, const char* defaultValue);
GS_API void C_SaveDataSetString(const char* key, const char* value);
// Copies up to capacity bytes into out and returns the size of the stored data, 0 when there is none.
GS_API uint32_t C_SaveDataGetBytes(const char* key, void* out, uint32_t capacity);
GS_API void C_SaveDataSetBytes(const char* key, const void* data, uint32_t size);
