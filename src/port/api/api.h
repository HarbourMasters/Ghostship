#pragma once

#ifdef __cplusplus
#define GS_API extern "C"
#else
#define GS_API extern
#endif

// Version of the mod API in these headers. It goes up when something is added to them. The mod template writes it
// to manifest.json as api_version, and the game doesn't load mods that need a newer version than its own.
// Definition structs only grow at the end. The C_Register* macros pass the size of the definition the mod was built
// with to the *Sized functions behind them, so fields added in later versions read as zero for older mods.
#define GS_API_VERSION 1
// Goes up when a change breaks mods built with older headers. The template writes it as code_version, and the game
// only loads mods built with its own.
#define GS_CODE_VERSION 2

// Declares a game asset by its path inside the game archive, e.g. "actors/mario/geo/mario_geo_left_hand".
#define C_ASSET(name, path) static const char name[] __attribute__((aligned(2))) = "__OTR__" path
