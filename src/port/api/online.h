#pragma once

#include <stdint.h>

#include "api.h"

// The player's online identity: an ed25519 key made once per install, proven to Satella on every connection.
// Without Satella (iOS, the web, or turned off in Dev Tools) the key is empty and these return FALSE.

#define C_PLAYER_KEY_HEX_LEN 64
#define C_SIGNATURE_SIZE 64

// TRUE while connected to Satella with the player key accepted. SatellaConnected and SatellaDisconnected report
// changes.
GS_API int C_IsOnline(void);
// This player's public key in hex, as other players see it on relay messages and in rooms. Empty without Satella.
GS_API const char* C_PlayerPublicKey(void);
// Signs data with the player's key, writing C_SIGNATURE_SIZE bytes to signature. Returns FALSE without a key.
GS_API int C_PlayerSign(const void* data, uint32_t size, uint8_t* signature);
// TRUE when signature is the signature of data by the player whose key is key_hex.
GS_API int C_VerifySignature(const char* key_hex, const void* data, uint32_t size, const uint8_t* signature);
