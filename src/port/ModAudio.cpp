#include "ModAudio.h"
#include "Engine.h"
#include <cstring>
#include <mutex>
#include <array>
#include <atomic>
#include <algorithm>

#define MOD_AUDIO_MAX_SOURCES 8
#define MOD_AUDIO_RING_SIZE 16384
#define MOD_AUDIO_RING_MASK (MOD_AUDIO_RING_SIZE - 1)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct ModAudioSource {
    std::atomic<bool> active{ false };
    std::mutex mutex;
    int16_t ring[MOD_AUDIO_RING_SIZE * 2];
    uint32_t write_pos = 0;
    uint32_t read_pos = 0;
};

static std::array<ModAudioSource, MOD_AUDIO_MAX_SOURCES> gSources;

extern "C" {

ModAudioHandle ModAudio_Register(void) {
    for (uint32_t i = 0; i < MOD_AUDIO_MAX_SOURCES; i++) {
        bool expected = false;
        if (gSources[i].active.compare_exchange_strong(expected, true)) {
            auto& src = gSources[i];
            std::lock_guard<std::mutex> lock(src.mutex);
            src.write_pos = 0;
            src.read_pos = 0;
            memset(src.ring, 0, sizeof(src.ring));
            return i + 1;
        }
    }
    return MOD_AUDIO_INVALID;
}

void ModAudio_Unregister(ModAudioHandle handle) {
    if (handle == MOD_AUDIO_INVALID || handle > MOD_AUDIO_MAX_SOURCES) {
        return;
    }
    gSources[handle - 1].active.store(false);
}

void ModAudio_SubmitSamples(ModAudioHandle handle, const int16_t* samples, uint32_t stereo_count) {
    if (handle == MOD_AUDIO_INVALID || handle > MOD_AUDIO_MAX_SOURCES) {
        return;
    }
    auto& src = gSources[handle - 1];
    if (!src.active.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(src.mutex);
    uint32_t available_space = MOD_AUDIO_RING_SIZE - (src.write_pos - src.read_pos);
    uint32_t to_write = MIN(stereo_count, available_space);
    for (uint32_t i = 0; i < to_write; i++) {
        uint32_t pos = src.write_pos & MOD_AUDIO_RING_MASK;
        src.ring[pos * 2] = samples[i * 2];
        src.ring[pos * 2 + 1] = samples[i * 2 + 1];
        src.write_pos++;
    }
}

uint32_t ModAudio_GetSampleRate(void) {
    return 32000;
}

uint32_t ModAudio_GetFrameSize(void) {
    return SAMPLES_HIGH;
}

uint32_t ModAudio_GetFillLevel(ModAudioHandle handle) {
    if (handle == MOD_AUDIO_INVALID || handle > MOD_AUDIO_MAX_SOURCES) {
        return 0;
    }
    auto& src = gSources[handle - 1];
    if (!src.active.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(src.mutex);
    return src.write_pos - src.read_pos;
}

} // extern "C"

void ModAudio_MixInto(int16_t* buf, uint32_t stereo_count) {
    for (auto& src : gSources) {
        if (!src.active.load()) {
            continue;
        }

        std::lock_guard<std::mutex> lock(src.mutex);
        uint32_t available = src.write_pos - src.read_pos;
        uint32_t to_read = MIN(available, stereo_count);

        for (uint32_t i = 0; i < to_read; i++) {
            uint32_t pos = src.read_pos & MOD_AUDIO_RING_MASK;
            int32_t l = (int32_t)buf[i * 2] + src.ring[pos * 2];
            int32_t r = (int32_t)buf[i * 2 + 1] + src.ring[pos * 2 + 1];
            buf[i * 2] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
            buf[i * 2 + 1] = (int16_t)(r > 32767 ? 32767 : r < -32768 ? -32768 : r);
            src.read_pos++;
        }
    }
}
