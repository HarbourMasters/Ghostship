#ifdef ENABLE_SCRIPTING
#include "port/api/online.h"

#ifdef USE_NETWORKING
#include "port/net/PlayerIdentity.h"
#include "port/net/SatellaClient.h"

#include <ship/utils/StringHelper.h>

#include <monocypher-ed25519.h>

#include <cctype>
#include <cstring>
#include <string>

extern "C" int C_IsOnline(void) {
    return Satella::Client::Instance().IsConnected() && Satella::Client::Instance().IsVerified();
}

extern "C" const char* C_PlayerPublicKey(void) {
    static const std::string key = [] {
        const auto pub = PlayerIdentity::GetPublicKey();
        return pub.empty() ? std::string() : StringHelper::BytesToHex(pub);
    }();
    return key.c_str();
}

extern "C" int C_PlayerSign(const void* data, uint32_t size, uint8_t* signature) {
    if (signature == nullptr || (data == nullptr && size != 0)) {
        return 0;
    }
    const auto sig = PlayerIdentity::Sign(static_cast<const uint8_t*>(data), size);
    if (sig.size() != C_SIGNATURE_SIZE) {
        return 0;
    }
    memcpy(signature, sig.data(), sig.size());
    return 1;
}

extern "C" int C_VerifySignature(const char* key_hex, const void* data, uint32_t size, const uint8_t* signature) {
    if (key_hex == nullptr || signature == nullptr || (data == nullptr && size != 0) ||
        strlen(key_hex) != C_PLAYER_KEY_HEX_LEN) {
        return 0;
    }
    for (const char* c = key_hex; *c != '\0'; c++) {
        if (!isxdigit(static_cast<unsigned char>(*c))) {
            return 0;
        }
    }
    const auto key = StringHelper::HexToBytes(key_hex);
    return crypto_ed25519_check(signature, key.data(), static_cast<const uint8_t*>(data), size) == 0;
}

#else

extern "C" int C_IsOnline(void) {
    return 0;
}
extern "C" const char* C_PlayerPublicKey(void) {
    return "";
}
extern "C" int C_PlayerSign(const void*, uint32_t, uint8_t*) {
    return 0;
}
extern "C" int C_VerifySignature(const char*, const void*, uint32_t, const uint8_t*) {
    return 0;
}

#endif
#endif
