#ifdef USE_NETWORKING
#include "PlayerIdentity.h"

#include "ship/Context.h"
#include "ship/config/Config.h"
#include "ship/utils/StringHelper.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <monocypher-ed25519.h>

#include <atomic>
#include <cstring>
#include <mutex>

namespace PlayerIdentity {

static std::mutex sMtx;
static std::atomic<bool> sInitialized{ false };
static uint8_t sPublicKey[32];
static uint8_t sSecretKey[64];

static bool FillRandom(uint8_t* buf, size_t size) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);

    const char* pers = "ghostship_player_identity";
    bool ok = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, reinterpret_cast<const unsigned char*>(pers),
                                    std::strlen(pers)) == 0 &&
              mbedtls_ctr_drbg_random(&drbg, buf, size) == 0;

    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
    return ok;
}

static void EnsureInitialized() {
    if (sInitialized.load(std::memory_order_acquire)) {
        return;
    }

    std::lock_guard<std::mutex> lock(sMtx);

    if (sInitialized.load(std::memory_order_relaxed)) {
        return;
    }

    auto config = Ship::Context::GetInstance()->GetConfig();
    std::string stored = config->GetString("Satella.PlayerSeed");

    uint8_t seed[32] = {};

    if (stored.size() == 64) {
        auto bytes = StringHelper::HexToBytes(stored);
        if (bytes.size() == 32) {
            std::copy(bytes.begin(), bytes.end(), seed);
        } else {
            stored.clear();
        }
    }

    if (stored.size() != 64) {
        if (!FillRandom(seed, sizeof(seed))) {
            return;
        }

        config->SetString("Satella.PlayerSeed", StringHelper::BytesToHex({ seed, seed + sizeof(seed) }));
        config->Save();
    }

    crypto_ed25519_key_pair(sSecretKey, sPublicKey, seed);
    crypto_wipe(seed, sizeof(seed));
    sInitialized.store(true, std::memory_order_release);
}

std::vector<uint8_t> GetPublicKey() {
    EnsureInitialized();

    if (!sInitialized.load(std::memory_order_relaxed)) {
        return {};
    }

    return { sPublicKey, sPublicKey + 32 };
}

std::vector<uint8_t> Sign(const uint8_t* data, size_t size) {
    EnsureInitialized();

    if (!sInitialized.load(std::memory_order_relaxed)) {
        return {};
    }

    std::vector<uint8_t> sig(64);
    crypto_ed25519_sign(sig.data(), sSecretKey, data, size);
    return sig;
}

} // namespace PlayerIdentity
#endif
