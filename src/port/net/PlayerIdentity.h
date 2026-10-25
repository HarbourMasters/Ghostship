#pragma once
#ifdef USE_NETWORKING

#include <cstddef>
#include <cstdint>
#include <vector>

// The player's ed25519 key pair, made once from a random seed kept in the config as "Satella.PlayerSeed".
namespace PlayerIdentity {

std::vector<uint8_t> GetPublicKey();
std::vector<uint8_t> Sign(const uint8_t* data, size_t size);

} // namespace PlayerIdentity
#endif
