#ifdef ENABLE_SCRIPTING

#include "port/ShipCompat.h"
#include "port/ShipInit.hpp"
#include "port/api/ModOwner.hpp"
#include "port/audio/ModSound.h"
#include "port/events/Events.h"

#include <ship/resource/File.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <spdlog/spdlog.h>

#include <libultraship.h>
extern "C" {
#include "port/api/sound.h"
#include "audio/external.h"
}

#include <string>
#include <unordered_map>
#include <vector>

// Everything here runs on the game thread; ModSound guards what the audio thread reads.
namespace {

constexpr int kNoSeq = -1;
// Forces the next tick to look at the level music again.
constexpr int kRecheck = -2;

struct Replacement {
    int sound;
    int32_t loopStart;
    std::string owner;
};

std::unordered_map<int, std::string> sSounds;
std::unordered_map<int, Replacement> sReplacements;
// The level sequence a replacement is playing for, or kNoSeq.
int sReplacing = kNoSeq;
// The level sequence seen on the last tick.
int sLastSeq = kRecheck;

int Register(const uint8_t* data, size_t size, const char* what) {
    std::vector<int16_t> samples;
    if (!ModSound::Decode(data, size, samples)) {
        SPDLOG_ERROR("Sound: {} is not a WAV, MP3 or FLAC file", what);
        return -1;
    }
    const int sound = ModSound::Add(std::move(samples));
    sSounds[sound] = ModOwner::Current();
    return sound;
}

int CurrentLevelSeq() {
    const u16 current = get_current_background_music();
    return current == 0xFFFF ? kNoSeq : (current & 0xFF);
}

void StopReplacement() {
    if (sReplacing != kNoSeq) {
        ModSound::StopMusic();
        sReplacing = kNoSeq;
    }
}

// SM64 keeps level music in a priority queue, and a sequence can come back without play_music (when cap music ends,
// say), so the replacement follows what is actually at the front of the queue instead of the events alone.
void FollowLevelMusic() {
    const int seq = CurrentLevelSeq();
    if (seq == sLastSeq) {
        return;
    }
    sLastSeq = seq;
    if (const auto it = sReplacements.find(seq); it != sReplacements.end()) {
        ModSound::PlayMusic(it->second.sound, it->second.loopStart);
        sReplacing = seq;
    } else {
        StopReplacement();
    }
}

void InitSoundApi() {
    REGISTER_LISTENER(GameLoopTick, EVENT_PRIORITY_LOW, [](IEvent*) { FollowLevelMusic(); });
    REGISTER_LISTENER(PlayMusicEvent, EVENT_PRIORITY_LOW, [](IEvent* event) {
        auto* play = reinterpret_cast<PlayMusicEvent*>(event);
        if (!event->Cancelled && play->player == SEQ_PLAYER_LEVEL) {
            sLastSeq = kRecheck; // the same sequence started again restarts its replacement
        }
    });
    REGISTER_LISTENER(FadeoutMusicEvent, EVENT_PRIORITY_LOW, [](IEvent* event) {
        auto* fade = reinterpret_cast<FadeoutMusicEvent*>(event);
        if (!event->Cancelled && sReplacing != kNoSeq && (*fade->seqId & 0xFF) == sReplacing) {
            StopReplacement();
        }
    });
    ModOwner::AddReleaser([](const std::string& mod) {
        std::erase_if(sReplacements, [&](const auto& entry) { return entry.second.owner == mod; });
        std::vector<int> sounds;
        for (const auto& [sound, owner] : sSounds) {
            if (owner == mod) {
                sounds.push_back(sound);
            }
        }
        for (int sound : sounds) {
            C_UnregisterSound(sound);
        }
        sLastSeq = kRecheck;
    });
}

RegisterShipInitFunc sInitSoundApi(InitSoundApi);

} // namespace

extern "C" int C_RegisterSound(const char* path) {
    if (path == nullptr) {
        return -1;
    }
    const auto file = ShipCompat::GetResourceManager()->GetArchiveManager()->LoadFile(path);
    if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr) {
        SPDLOG_ERROR("Sound: could not read {}", path);
        return -1;
    }
    return Register((const uint8_t*)file->Buffer->data(), file->Buffer->size(), path);
}

extern "C" int C_RegisterSoundData(const uint8_t* data, uint32_t size) {
    return Register(data, size, "the data");
}

extern "C" void C_UnregisterSound(int sound) {
    if (sSounds.erase(sound) == 0) {
        return;
    }
    std::erase_if(sReplacements, [sound](const auto& entry) { return entry.second.sound == sound; });
    ModSound::Remove(sound);
}

extern "C" uint32_t C_SoundLength(int sound) {
    return (uint32_t)ModSound::Length(sound);
}

extern "C" int C_PlaySound(int sound, float volume, float pan) {
    return ModSound::Play(sound, volume, pan);
}

extern "C" void C_StopSound(int voice) {
    ModSound::Stop(voice);
}

extern "C" int C_IsSoundPlaying(int voice) {
    return ModSound::Playing(voice);
}

extern "C" void C_PlayMusic(int sound, int32_t loop_start) {
    sReplacing = kNoSeq;
    ModSound::PlayMusic(sound, loop_start);
}

extern "C" void C_StopMusic(void) {
    sReplacing = kNoSeq;
    ModSound::StopMusic();
}

extern "C" void C_ReplaceMusic(uint8_t seq_id, int sound, int32_t loop_start) {
    if (sound < 0 || !sSounds.contains(sound)) {
        sReplacements.erase(seq_id);
    } else {
        sReplacements[seq_id] = { sound, loop_start, ModOwner::Current() };
    }
    // Takes effect at once when that sequence is playing.
    if (CurrentLevelSeq() == seq_id) {
        StopReplacement();
        sLastSeq = kRecheck;
    }
}

#endif
