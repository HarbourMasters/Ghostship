#pragma once
#ifdef USE_NETWORKING

#include <cstdint>
#include <vector>

namespace PlayerIdentity {

std::vector<uint8_t> GetPublicKey();
std::vector<uint8_t> Sign(const uint8_t* data, size_t size);

} // namespace PlayerIdentity
#endif
