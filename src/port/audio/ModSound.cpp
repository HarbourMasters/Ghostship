#include "ModSound.h"

// dr_libs is compiled once, in importer/AudioSampleFactory.cpp.
#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace {

constexpr int kSampleRate = 32000;

using Samples = std::vector<int16_t>;

struct Voice {
    int id;
    std::shared_ptr<const Samples> samples;
    size_t frame = 0;
    float left = 1.0f;
    float right = 1.0f;
    bool music = false;
    int64_t loopStart = -1;
};

std::mutex sMutex;
std::unordered_map<int, std::shared_ptr<const Samples>> sSounds;
std::vector<Voice> sVoices;
int sNextSound = 1;
int sNextVoice = 1;

void ToStereo(const int16_t* pcm, uint64_t frames, unsigned channels, unsigned rate, Samples& out) {
    if (frames == 0 || channels == 0 || rate == 0) {
        out.clear();
        return;
    }
    const uint64_t count = frames * kSampleRate / rate;
    out.resize((size_t)count * 2);
    for (uint64_t i = 0; i < count; i++) {
        const double at = (double)i * rate / kSampleRate;
        const uint64_t a = std::min<uint64_t>((uint64_t)at, frames - 1);
        const uint64_t b = std::min<uint64_t>(a + 1, frames - 1);
        const double t = at - (double)a;
        for (unsigned c = 0; c < 2; c++) {
            const unsigned src = std::min(c, channels - 1);
            const double sa = pcm[a * channels + src];
            const double sb = pcm[b * channels + src];
            out[(size_t)i * 2 + c] = (int16_t)(sa + (sb - sa) * t);
        }
    }
}

int16_t Clamp(int value) {
    return (int16_t)std::clamp(value, -0x8000, 0x7FFF);
}

} // namespace

bool ModSound::Decode(const uint8_t* data, size_t size, std::vector<int16_t>& out) {
    if (data == nullptr || size < 4) {
        return false;
    }
    unsigned channels = 0;
    unsigned rate = 0;
    if (memcmp(data, "RIFF", 4) == 0) {
        drwav_uint64 frames = 0;
        int16_t* pcm = drwav_open_memory_and_read_pcm_frames_s16(data, size, &channels, &rate, &frames, nullptr);
        if (pcm == nullptr) {
            return false;
        }
        ToStereo(pcm, frames, channels, rate, out);
        drwav_free(pcm, nullptr);
        return true;
    }
    if (memcmp(data, "fLaC", 4) == 0) {
        drflac_uint64 frames = 0;
        int16_t* pcm = drflac_open_memory_and_read_pcm_frames_s16(data, size, &channels, &rate, &frames, nullptr);
        if (pcm == nullptr) {
            return false;
        }
        ToStereo(pcm, frames, channels, rate, out);
        drflac_free(pcm, nullptr);
        return true;
    }
    drmp3_config config;
    drmp3_uint64 mp3Frames = 0;
    int16_t* pcm = drmp3_open_memory_and_read_pcm_frames_s16(data, size, &config, &mp3Frames, nullptr);
    if (pcm == nullptr) {
        return false;
    }
    ToStereo(pcm, mp3Frames, config.channels, config.sampleRate, out);
    drmp3_free(pcm, nullptr);
    return true;
}

int ModSound::Add(std::vector<int16_t> samples) {
    std::lock_guard<std::mutex> lock(sMutex);
    const int sound = sNextSound++;
    sSounds[sound] = std::make_shared<const Samples>(std::move(samples));
    return sound;
}

void ModSound::Remove(int sound) {
    std::lock_guard<std::mutex> lock(sMutex);
    auto it = sSounds.find(sound);
    if (it == sSounds.end()) {
        return;
    }
    const Samples* samples = it->second.get();
    std::erase_if(sVoices, [&](const Voice& voice) { return voice.samples.get() == samples; });
    sSounds.erase(it);
}

size_t ModSound::Length(int sound) {
    std::lock_guard<std::mutex> lock(sMutex);
    auto it = sSounds.find(sound);
    return it != sSounds.end() ? it->second->size() / 2 : 0;
}

int ModSound::Play(int sound, float volume, float pan) {
    std::lock_guard<std::mutex> lock(sMutex);
    auto it = sSounds.find(sound);
    if (it == sSounds.end() || it->second->empty()) {
        return -1;
    }
    volume = std::clamp(volume, 0.0f, 1.0f);
    pan = std::clamp(pan, -1.0f, 1.0f);
    Voice voice{ sNextVoice++, it->second };
    voice.left = volume * std::min(1.0f, 1.0f - pan);
    voice.right = volume * std::min(1.0f, 1.0f + pan);
    sVoices.push_back(voice);
    return voice.id;
}

void ModSound::Stop(int voice) {
    std::lock_guard<std::mutex> lock(sMutex);
    std::erase_if(sVoices, [voice](const Voice& v) { return v.id == voice; });
}

bool ModSound::Playing(int voice) {
    std::lock_guard<std::mutex> lock(sMutex);
    return std::any_of(sVoices.begin(), sVoices.end(), [voice](const Voice& v) { return v.id == voice; });
}

void ModSound::PlayMusic(int sound, int loopStart) {
    std::lock_guard<std::mutex> lock(sMutex);
    std::erase_if(sVoices, [](const Voice& v) { return v.music; });
    auto it = sSounds.find(sound);
    if (it == sSounds.end() || it->second->empty()) {
        return;
    }
    Voice voice{ sNextVoice++, it->second };
    voice.music = true;
    voice.loopStart = loopStart >= 0 && (size_t)loopStart < it->second->size() / 2 ? loopStart : -1;
    sVoices.push_back(voice);
}

void ModSound::StopMusic() {
    std::lock_guard<std::mutex> lock(sMutex);
    std::erase_if(sVoices, [](const Voice& v) { return v.music; });
}

bool ModSound::MusicActive() {
    std::lock_guard<std::mutex> lock(sMutex);
    return std::any_of(sVoices.begin(), sVoices.end(), [](const Voice& v) { return v.music; });
}

void ModSound::Mix(int16_t* stereo, int frames, float musicVolume, float sfxVolume) {
    std::lock_guard<std::mutex> lock(sMutex);
    for (Voice& voice : sVoices) {
        const Samples& samples = *voice.samples;
        const size_t length = samples.size() / 2;
        const float gain = voice.music ? musicVolume : sfxVolume;
        for (int i = 0; i < frames && voice.frame < length; i++) {
            stereo[i * 2] = Clamp(stereo[i * 2] + (int)(samples[voice.frame * 2] * voice.left * gain));
            stereo[i * 2 + 1] = Clamp(stereo[i * 2 + 1] + (int)(samples[voice.frame * 2 + 1] * voice.right * gain));
            if (++voice.frame >= length && voice.loopStart >= 0) {
                voice.frame = (size_t)voice.loopStart;
            }
        }
    }
    std::erase_if(sVoices, [](const Voice& v) { return v.frame >= v.samples->size() / 2; });
}
