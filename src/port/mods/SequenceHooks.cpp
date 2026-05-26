#include <mutex>
#include <unordered_map>

#include <ship/events/EventSystem.h>

#include "port/ShipInit.hpp"
#include "port/mods/SequenceHooks.h"
#include "port/events/Events.h"
#include "port/importer/AudioSequenceFactory.h"

extern "C" {
#include "audio/internal.h"
}

#define SEQ_PLAYER_MAX 3
#define SEQ_ID_NONE 0xFF

static std::mutex sRemapMtx;
static std::unordered_map<uint16_t, uint16_t> sSeqRemap;
static uint16_t sCurrentSeqId[SEQ_PLAYER_MAX];

static uint16_t ApplyRemap(uint16_t seqId) {
    std::lock_guard<std::mutex> lock(sRemapMtx);
    auto it = sSeqRemap.find(seqId);
    return (it != sSeqRemap.end()) ? it->second : seqId;
}

static void OnPlayMusic(IEvent* ev) {
    auto* event = reinterpret_cast<PlayMusicEvent*>(ev);
    u16 remapped = ApplyRemap(*event->seqArgs & 0xFF);
    *event->seqArgs = (*event->seqArgs & 0xFF00) | (remapped & 0xFF);
    if (event->player < SEQ_PLAYER_MAX) {
        sCurrentSeqId[event->player] = remapped;
    }
}

static void OnStopMusic(IEvent* ev) {
    auto* event = reinterpret_cast<StopMusicEvent*>(ev);
    *event->seqId = ApplyRemap(*event->seqId & 0xFF);
    if (SEQ_PLAYER_LEVEL < SEQ_PLAYER_MAX) {
        sCurrentSeqId[SEQ_PLAYER_LEVEL] = SEQ_ID_NONE;
    }
}

static void OnFadeoutMusic(IEvent* ev) {
    auto* event = reinterpret_cast<FadeoutMusicEvent*>(ev);
    *event->seqId = ApplyRemap(*event->seqId & 0xFF);
}

void Sequence_AddRemap(uint16_t from, uint16_t to) {
    std::lock_guard<std::mutex> lock(sRemapMtx);
    sSeqRemap[from] = to;
}

void Sequence_RemoveRemap(uint16_t from) {
    std::lock_guard<std::mutex> lock(sRemapMtx);
    sSeqRemap.erase(from);
}

uint16_t Sequence_GetCurrentSeqId(uint8_t seqPlayId) {
    if (seqPlayId >= SEQ_PLAYER_MAX) {
        return SEQ_ID_NONE;
    }
    return sCurrentSeqId[seqPlayId];
}

bool Sequence_IsMapped(uint16_t seqId) {
    std::lock_guard<std::mutex> lock(sRemapMtx);
    return sSeqRemap.find(seqId) != sSeqRemap.end();
}

static void OnSeqLayerPreNote(IEvent* ev) {
    auto* event = reinterpret_cast<SeqLayerPreNoteEvent*>(ev);
    struct SequenceChannel* chan = event->channel;
    uint8_t seqId = chan->seqPlayer->seqId;

    // Find channel index within the sequence player (needed for stereo L/R selection).
    uint8_t chanIdx = 0;
    for (int i = 0; i < CHANNELS_MAX; i++) {
        if (chan->seqPlayer->channels[i] == chan) {
            chanIdx = (uint8_t)i;
            break;
        }
    }

    AudioBankSound* sound = SM64::AudioSequenceFactoryV0::GetStreamedSound(seqId, chanIdx);
    if (!sound) return;

    event->layer->sound = sound;
}

static void SequenceHooks_Init() {
    for (int i = 0; i < SEQ_PLAYER_MAX; i++) {
        sCurrentSeqId[i] = SEQ_ID_NONE;
    }
    REGISTER_LISTENER(PlayMusicEvent, EVENT_PRIORITY_HIGH, OnPlayMusic);
    REGISTER_LISTENER(StopMusicEvent, EVENT_PRIORITY_HIGH, OnStopMusic);
    REGISTER_LISTENER(FadeoutMusicEvent, EVENT_PRIORITY_HIGH, OnFadeoutMusic);
    REGISTER_LISTENER(SeqLayerPreNoteEvent, EVENT_PRIORITY_HIGH, OnSeqLayerPreNote);
}

static RegisterShipInitFunc sInitFunc(SequenceHooks_Init);
