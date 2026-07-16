#include "AudioSequenceFactory.h"
#include "port/ShipCompat.h"
#include "port/importer/types/AudioSequence.h"
#include "port/importer/types/AudioSample.h"
#include "spdlog/spdlog.h"
#include "port/Engine.h"
#include <tinyxml2.h>
#include "port/importer/types/AudioBank.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <ship/utils/binarytools/endianness.h>
#include <ship/utils/binarytools/BinaryWriter.h>
#include <dr_mp3.h>
#include <dr_wav.h>
#include <dr_flac.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BSWAP16_BE(x) (x)
#else
#define BSWAP16_BE(x) ((int16_t)(((uint16_t)(x) << 8) | ((uint16_t)(x) >> 8)))
#endif

static constexpr uint8_t STREAMED_BANK_ID = 200;

// == M64 opcode helpers (big-endian, mirrors Starship SequenceFactory) =========
static void M64_Byte(Ship::BinaryWriter& w, uint8_t op) {
    w.Write(op);
}
static void M64_U16(Ship::BinaryWriter& w, uint8_t op, uint16_t a) {
    w.Write(op);
    w.Write(a);
}
static void M64_U8(Ship::BinaryWriter& w, uint8_t op, uint8_t a) {
    w.Write(op);
    w.Write(a);
}

// seq_initchannels mask (big-endian u16)
static void M64_InitChan(Ship::BinaryWriter& w, uint16_t mask) {
    M64_U16(w, 0xD7, mask);
}
// seq_settempo bpm
static void M64_Tempo(Ship::BinaryWriter& w, uint8_t bpm) {
    M64_U8(w, 0xDD, bpm);
}
// seq_startchannel ch, off  /  chan_setlayer layer, off
static void M64_LdOff(Ship::BinaryWriter& w, uint8_t idx, uint16_t off) {
    M64_U16(w, (uint8_t)(0x90 | idx), off);
}
// seq_delay / chan_delay (1 or 2-byte compressed)
static void M64_Delay(Ship::BinaryWriter& w, uint16_t d) {
    if (d > 0x7F) {
        w.Write((uint8_t)0xFD);
        w.Write((uint16_t)(d | 0x8000));
    } else {
        w.Write((uint8_t)0xFD);
        w.Write((uint8_t)d);
    }
}
// seq_jump / chan_jump / layer_jump
static void M64_Jump(Ship::BinaryWriter& w, uint16_t off) {
    M64_U16(w, 0xFB, off);
}
// chan_largenoteson / layer_legato
static void M64_Legato(Ship::BinaryWriter& w) {
    M64_Byte(w, 0xC4);
}
// chan_setvol v
static void M64_VolChan(Ship::BinaryWriter& w, uint8_t v) {
    M64_U8(w, 0xDF, v);
}
// chan_pan p  (0=full-left, 64=centre, 127=full-right)
static void M64_Pan(Ship::BinaryWriter& w, uint8_t p) {
    M64_U8(w, 0xDD, p);
}
// chan_setbank idx
static void M64_SetBank(Ship::BinaryWriter& w, uint8_t idx) {
    M64_U8(w, 0xC6, idx);
}
// chan_setinstrument idx
static void M64_SetInstr(Ship::BinaryWriter& w, uint8_t idx) {
    M64_U8(w, 0xC1, idx);
}
// seq_setmutebhv v  (overrides seqPlayer->muteBehavior set by init_sequence_player)
static void M64_SeqSetMuteBhv(Ship::BinaryWriter& w, uint8_t v) {
    M64_U8(w, 0xD3, v);
}
// chan_setmutebhv v  (sets seqChannel->muteBehavior)
static void M64_ChanSetMuteBhv(Ship::BinaryWriter& w, uint8_t v) {
    M64_U8(w, 0xCA, v);
}
// chan_delay1
static void M64_Delay1(Ship::BinaryWriter& w) {
    M64_Byte(w, 0xFE);
}
// layer note: semitone, delay (2-byte if > 0x7F), velocity, gate
static void M64_NoteDVG(Ship::BinaryWriter& w, uint8_t semi, uint16_t d, uint8_t vel, uint8_t gate) {
    w.Write(semi);
    if (d > 0x7F)
        w.Write((uint16_t)(d | 0x8000));
    else
        w.Write((uint8_t)d);
    w.Write(vel);
    w.Write(gate);
}

static std::vector<uint8_t> BuildLoopM64() {
    Ship::BinaryWriter w;
    w.SetEndianness(Ship::Endianness::Big);

    // == Sequence header ========================================================
    M64_InitChan(w, 0x0001);    // initchannels ch=0
    M64_SeqSetMuteBhv(w, 0x00); // clear all seq-level mute flags; mute silenced via synthesis.c

    uint16_t seqChanOff = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 0, 0x0000);                           // startchannel ch=0 (placeholder)
    uint16_t seqLoopPt = (uint16_t)w.GetBaseAddress(); // loop point AFTER startchannel
    M64_Delay1(w);                                     // seq_delay1
    M64_Jump(w, seqLoopPt);                            // jump → loop

    // == Channel 0 =============================================================
    uint16_t chanStart = (uint16_t)w.GetBaseAddress();
    M64_Legato(w);               // largenoteson
    M64_ChanSetMuteBhv(w, 0x00); // no mute flags: reclaim_notes must NOT kill this note;
                                 // mute silencing is handled in synthesis.c instead
    M64_VolChan(w, 0x7F);        // setvol 127
    M64_SetBank(w, 0x00);        // setbank 0 (→ STREAMED_BANK_ID)
    M64_SetInstr(w, 0x00);       // instrument 0 (non-drum path)

    // chan_setlayer runs ONCE; loop starts after it so we don't call
    // seq_channel_set_layer every tick (which would decay the note each tick).
    uint16_t chanLayerOff = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 0, 0x0000);                            // setlayer 0 (placeholder)
    uint16_t chanLoopPt = (uint16_t)w.GetBaseAddress(); // loop point AFTER setlayer
    M64_Delay1(w);                                      // chan_delay1
    M64_Jump(w, chanLoopPt);                            // jump → loop

    // == Layer 0 ===============================================================
    // The 0xC4 (layer_somethingon) sets continuousNotes=TRUE and decays any
    // existing note.  chan_setlayer points HERE (before the note opcode) so the
    // first run executes 0xC4 once, then falls through to the note opcode.
    // The jump target (layerStart) sits AFTER the 0xC4, so every subsequent
    // loop skips it: continuousNotes stays TRUE and the same note object is
    // reused, preserving samplePosInt across the 32766-tick boundary.
    // gate=0x00 → layer->duration=0 → early-decay check (delay<=duration)
    // is never true, so the note is never force-decayed mid-cycle.
    uint16_t layerLegatoOff = (uint16_t)w.GetBaseAddress();
    M64_Legato(w);                                      // 0xC4: continuousNotes=TRUE (first run only)
    uint16_t layerStart = (uint16_t)w.GetBaseAddress(); // jump target: note opcode
    M64_NoteDVG(w, 39, 0x7FFE, 0x7F, 0x00);             // note G#3, max delay, vel 127, gate 0
    M64_Jump(w, layerStart);                            // loop back — skips the 0xC4

    // == Back-patch offsets ====================================================
    w.Seek((int32_t)seqChanOff, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 0, chanStart);
    w.Seek((int32_t)chanLayerOff, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 0, layerLegatoOff); // ← first run: hits 0xC4, then note opcode

    auto chars = w.ToVector();
    return std::vector<uint8_t>(chars.begin(), chars.end());
}

static std::vector<uint8_t> BuildStereoM64() {
    Ship::BinaryWriter w;
    w.SetEndianness(Ship::Endianness::Big);

    // == Sequence header ========================================================
    M64_InitChan(w, 0x0003);    // initchannels ch0+ch1
    M64_SeqSetMuteBhv(w, 0x00); // clear all seq-level mute flags; mute silenced via synthesis.c

    // Both startchannel calls run ONCE; loop starts after them.
    uint16_t seqCh0Off = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 0, 0x0000); // startchannel ch=0 (placeholder)
    uint16_t seqCh1Off = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 1, 0x0000);                           // startchannel ch=1 (placeholder)
    uint16_t seqLoopPt = (uint16_t)w.GetBaseAddress(); // loop point AFTER both startchannels
    M64_Delay1(w);                                     // seq_delay1
    M64_Jump(w, seqLoopPt);                            // jump → loop

    // == Channel 0  (Left, pan=0) ===============================================
    uint16_t ch0Start = (uint16_t)w.GetBaseAddress();
    M64_Legato(w);               // largenoteson
    M64_ChanSetMuteBhv(w, 0x00); // no mute flags: reclaim_notes must NOT kill this note
    M64_VolChan(w, 0x7F);        // setvol 127
    M64_Pan(w, 0x00);            // pan full-left
    M64_SetBank(w, 0x00);        // setbank 0 (→ STREAMED_BANK_ID)
    M64_SetInstr(w, 0x00);       // instrument 0 = L sample
    uint16_t ch0LayOff = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 0, 0x0000);                           // setlayer 0 (placeholder)
    uint16_t ch0LoopPt = (uint16_t)w.GetBaseAddress(); // loop point AFTER setlayer
    M64_Delay1(w);                                     // chan_delay1
    M64_Jump(w, ch0LoopPt);                            // jump → loop

    // == Channel 1  (Right, pan=127) ============================================
    uint16_t ch1Start = (uint16_t)w.GetBaseAddress();
    M64_Legato(w);               // largenoteson
    M64_ChanSetMuteBhv(w, 0x00); // no mute flags: reclaim_notes must NOT kill this note
    M64_VolChan(w, 0x7F);        // setvol 127
    M64_Pan(w, 0x7F);            // pan full-right
    M64_SetBank(w, 0x00);        // setbank 0 (→ STREAMED_BANK_ID)
    M64_SetInstr(w, 0x01);       // instrument 1 = R sample
    uint16_t ch1LayOff = (uint16_t)w.GetBaseAddress();
    M64_LdOff(w, 0, 0x0000);                           // setlayer 0 (placeholder)
    uint16_t ch1LoopPt = (uint16_t)w.GetBaseAddress(); // loop point AFTER setlayer
    M64_Delay1(w);                                     // chan_delay1
    M64_Jump(w, ch1LoopPt);                            // jump → loop

    // == Layer 0  (L note) =====================================================
    // First run hits 0xC4 (sets continuousNotes=TRUE), then falls through to note.
    // The jump target l0Start is AFTER 0xC4, so loops skip it: same note object
    // continues across the 32766-tick boundary, preserving samplePosInt.
    uint16_t l0LegatoOff = (uint16_t)w.GetBaseAddress();
    M64_Legato(w); // 0xC4: continuousNotes=TRUE (first run only)
    uint16_t l0Start = (uint16_t)w.GetBaseAddress();
    M64_NoteDVG(w, 39, 0x7FFE, 0x7F, 0x00); // gate=0 → never early-decay
    M64_Jump(w, l0Start);

    // == Layer 1  (R note) =====================================================
    uint16_t l1LegatoOff = (uint16_t)w.GetBaseAddress();
    M64_Legato(w); // 0xC4: continuousNotes=TRUE (first run only)
    uint16_t l1Start = (uint16_t)w.GetBaseAddress();
    M64_NoteDVG(w, 39, 0x7FFE, 0x7F, 0x00);
    M64_Jump(w, l1Start);

    // == Back-patch offsets ====================================================
    w.Seek((int32_t)seqCh0Off, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 0, ch0Start);
    w.Seek((int32_t)seqCh1Off, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 1, ch1Start);
    w.Seek((int32_t)ch0LayOff, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 0, l0LegatoOff); // ← first run: hits 0xC4, then note opcode
    w.Seek((int32_t)ch1LayOff, Ship::SeekOffsetType::Start);
    M64_LdOff(w, 0, l1LegatoOff); // ← first run: hits 0xC4, then note opcode

    auto chars = w.ToVector();
    return std::vector<uint8_t>(chars.begin(), chars.end());
}

namespace {

struct CustomSeqState {
    std::shared_ptr<SM64::AudioSample> audioSample;

    // Mono path (channels == 1)
    AudioBankSound bankSound{};

    // Stereo path (channels == 2) — deinterleaved L + R PCM
    std::vector<int16_t> lPcm, rPcm;
    AudioBankSample lSampleData{}, rSampleData{};
    AdpcmLoop lLoop{}, rLoop{};
    AdpcmBook lBook{}, rBook{};
    AudioBankSound lSound{}, rSound{};

    AdsrEnvelope envelope[2]{};
    Instrument instruments[2]{};     // [0] = mono or L,  [1] = R (stereo only)
    Instrument* instrumentPtrs[2]{}; // parallel pointers into instruments[]

    CtlEntry ctlEntry{};
    std::vector<uint8_t> m64Data;
    std::vector<uint8_t> bankIds;
    AudioSequenceData seqData{};
};

static std::mutex sMp3Mtx;
static std::unordered_map<uint8_t, std::unique_ptr<CustomSeqState>> sMp3States;

} // namespace

static std::atomic<AudioBankSound*> sStreamSounds[256];
static std::atomic<AudioBankSound*> sStreamSoundsR[256];
static std::atomic<AudioSequenceData*> sStreamSeqData[256];

void SM64::AudioSequenceFactoryV0::RegisterSample(uint8_t seqId, std::shared_ptr<SM64::AudioSample> sample) {
    auto state = std::make_unique<CustomSeqState>();
    state->audioSample = sample;

    // ADSR envelope: instant attack to full volume, then hang forever (shared by both paths)
    state->envelope[0].delay = BSWAP16_BE(1);
    state->envelope[0].arg = BSWAP16_BE(32767);
    state->envelope[1].delay = BSWAP16_BE(-1); // ADSR_HANG
    state->envelope[1].arg = 0;

    const float tuning = (float)sample->mData.sampleRate / 32000.0f;

    if (sample->mData.channels == 2) {
        // == Stereo path: deinterleave L/R and set up two instruments ==========
        const auto* src = reinterpret_cast<const int16_t*>(sample->mData.sampleAddr);
        const size_t frames = sample->mData.numFrames;

        state->lPcm.resize(frames);
        state->rPcm.resize(frames);
        for (size_t i = 0; i < frames; i++) {
            state->lPcm[i] = src[i * 2 + 0];
            state->rPcm[i] = src[i * 2 + 1];
        }

        auto initSample = [&](AudioBankSample& sd, AdpcmLoop& lp, AdpcmBook& bk, std::vector<int16_t>& pcm) {
            lp = { 0, (uint32_t)frames, 0xFFFFFFFFu, 0, nullptr };
            bk = { 0, 0, nullptr };
            sd.unused = 0;
            sd.loaded = 1;
            sd.sampleAddr = reinterpret_cast<uint8_t*>(pcm.data());
            sd.loop = &lp;
            sd.book = &bk;
            sd.sampleSize = 0;
            sd.codec = 5; // CODEC_S16
            sd.numFrames = (uint32_t)frames;
            sd.channels = 1;
            sd.sampleRate = sample->mData.sampleRate;
        };
        initSample(state->lSampleData, state->lLoop, state->lBook, state->lPcm);
        initSample(state->rSampleData, state->rLoop, state->rBook, state->rPcm);

        state->lSound = { &state->lSampleData, tuning };
        state->rSound = { &state->rSampleData, tuning };

        for (int i = 0; i < 2; i++) {
            state->instruments[i].loaded = 1;
            state->instruments[i].normalRangeLo = 0;
            state->instruments[i].normalRangeHi = 127;
            state->instruments[i].releaseRate = 0;
            state->instruments[i].envelope = state->envelope;
        }
        state->instruments[0].normalNotesSound = state->lSound;
        state->instruments[1].normalNotesSound = state->rSound;
        state->instrumentPtrs[0] = &state->instruments[0];
        state->instrumentPtrs[1] = &state->instruments[1];

        state->ctlEntry.numInstruments = 2;
        state->m64Data = BuildStereoM64();
    } else {
        // == Mono path =========================================================
        state->bankSound = { &sample->mData, tuning };

        state->instruments[0].loaded = 1;
        state->instruments[0].normalRangeLo = 0;
        state->instruments[0].normalRangeHi = 127;
        state->instruments[0].releaseRate = 0;
        state->instruments[0].envelope = state->envelope;
        state->instruments[0].normalNotesSound = state->bankSound;
        state->instrumentPtrs[0] = &state->instruments[0];

        state->ctlEntry.numInstruments = 1;
        state->m64Data = BuildLoopM64();
    }

    state->ctlEntry.bankId = STREAMED_BANK_ID;
    state->ctlEntry.numDrums = 0;
    state->ctlEntry.instruments = state->instrumentPtrs;
    state->ctlEntry.drums = nullptr;

    state->bankIds = { STREAMED_BANK_ID };

    state->seqData.bankCount = 1;
    state->seqData.banks = state->bankIds.data();
    state->seqData.data = state->m64Data.data();
    state->seqData.id = seqId;

    auto* engine = GameEngine::Instance;
    if (!engine) {
        SPDLOG_ERROR("AudioSequenceFactory: GameEngine not ready for seqId 0x{:02X}", seqId);
        return;
    }

    if (STREAMED_BANK_ID < (int)engine->banksTable.size())
        engine->banksTable[STREAMED_BANK_ID] = reinterpret_cast<CtlEntry*>(&state->ctlEntry);
    if (seqId < (int)engine->audioSequenceTable.size())
        engine->audioSequenceTable[seqId] = reinterpret_cast<AudioSequenceData*>(&state->seqData);
    if (seqId < (int)engine->sequenceTable.size())
        engine->sequenceTable[seqId] = "__STREAMED__";

    bool isStereo = (sample->mData.channels == 2);
    sStreamSounds[seqId].store(isStereo ? &state->lSound : &state->bankSound, std::memory_order_release);
    sStreamSoundsR[seqId].store(isStereo ? &state->rSound : nullptr, std::memory_order_release);
    sStreamSeqData[seqId].store(reinterpret_cast<AudioSequenceData*>(&state->seqData), std::memory_order_release);

    std::lock_guard<std::mutex> lk(sMp3Mtx);
    sMp3States[seqId] = std::move(state);
}

AudioBankSound* SM64::AudioSequenceFactoryV0::GetStreamedSound(uint8_t seqId, uint8_t channelIdx) {
    if (channelIdx == 1) {
        auto* r = sStreamSoundsR[seqId].load(std::memory_order_acquire);
        if (r) {
            return r;
        }
    }
    return sStreamSounds[seqId].load(std::memory_order_acquire);
}

AudioSequenceData* SM64::AudioSequenceFactoryV0::GetStreamedSeqData(uint8_t seqId) {
    return sStreamSeqData[seqId].load(std::memory_order_acquire);
}

std::shared_ptr<Ship::IResource>
SM64::AudioSequenceFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                           std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    SPDLOG_INFO("Path: '{}'", initData->Path);

    std::shared_ptr<AudioSequence> bank = std::make_shared<AudioSequence>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint8_t id = reader->ReadUInt32();
    size_t bankCount = reader->ReadUInt32();
    for (size_t i = 0; i < bankCount; i++) {
        std::string bankName = reader->ReadString();
        bank->banks.push_back(GameEngine::GetBankIdByName(bankName));
    }

    size_t sampleSize = reader->ReadUInt32();
    for (size_t i = 0; i < sampleSize; i++) {
        bank->sampleData.push_back(reader->ReadUByte());
    }

    bank->mData.bankCount = bankCount;
    bank->mData.banks = bank->banks.data();
    bank->mData.data = bank->sampleData.data();
    bank->mData.id = id;

    return bank;
}

std::shared_ptr<Ship::IResource>
SM64::AudioSequenceXMLFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                              std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto child = std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement();

    // == Streamed audio replacement (mod CustomFormat) =========================
    const char* customFormat = child->Attribute("CustomFormat");
    const char* audioPath = child->Attribute("Path");
    int seqId = child->IntAttribute("ID", -1);

    if (customFormat != nullptr && audioPath != nullptr && seqId >= 0) {
        auto audioFile = ShipCompat::GetResourceManager()->GetArchiveManager()->LoadFile(audioPath);
        if (!audioFile || !audioFile->Buffer) {
            SPDLOG_ERROR("AudioSequenceFactory: could not load '{}'", audioPath);
            return nullptr;
        }

        std::vector<uint8_t> buf(audioFile->Buffer->begin(), audioFile->Buffer->end());
        auto sample = std::make_shared<SM64::AudioSample>();
        sample->loop.state = nullptr;
        sample->book = { 0, 0, nullptr };
        sample->mData.loop = &sample->loop;
        sample->mData.book = &sample->book;

        bool decoded = false;

        if (strcmp(customFormat, "mp3") == 0) {
            drmp3_config cfg{};
            drmp3_uint64 totalFrames = 0;
            drmp3_int16* raw =
                drmp3_open_memory_and_read_pcm_frames_s16(buf.data(), buf.size(), &cfg, &totalFrames, nullptr);
            if (raw && totalFrames > 0) {
                sample->sampleAddr.resize(totalFrames * sizeof(int16_t));
                auto* dst = reinterpret_cast<int16_t*>(sample->sampleAddr.data());
                if (cfg.channels == 1) {
                    for (size_t i = 0; i < (size_t)totalFrames; i++)
                        dst[i] = raw[i];
                } else {
                    for (size_t i = 0; i < (size_t)totalFrames; i++)
                        dst[i] = (int16_t)(((int32_t)raw[i * cfg.channels] + (int32_t)raw[i * cfg.channels + 1]) / 2);
                }
                drmp3_free(raw, nullptr);
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)totalFrames;
                sample->loop.count = 0xFFFFFFFFu;
                sample->mData.sampleAddr = sample->sampleAddr.data();
                sample->mData.numFrames = (uint32_t)totalFrames;
                sample->mData.channels = 1;
                sample->mData.codec = 5;
                sample->mData.sampleRate = cfg.sampleRate;
                sample->mData.loaded = 1;
                decoded = true;
            }
        } else if (strcmp(customFormat, "wav") == 0 || strcmp(customFormat, "aiff") == 0) {
            drwav wav;
            if (drwav_init_memory(&wav, buf.data(), buf.size(), nullptr)) {
                drwav_uint64 numFrames = wav.totalPCMFrameCount;
                uint32_t channels = wav.channels;
                sample->sampleAddr.resize(numFrames * channels * sizeof(int16_t));
                drwav_read_pcm_frames_s16(&wav, numFrames, reinterpret_cast<drwav_int16*>(sample->sampleAddr.data()));
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)numFrames;
                sample->loop.count = 0xFFFFFFFFu;
                sample->mData.sampleAddr = sample->sampleAddr.data();
                sample->mData.numFrames = (uint32_t)numFrames;
                sample->mData.channels = channels;
                sample->mData.codec = 5;
                sample->mData.sampleRate = wav.sampleRate;
                sample->mData.loaded = 1;
                drwav_uninit(&wav);
                decoded = true;
            }
        } else if (strcmp(customFormat, "flac") == 0) {
            drflac* flac = drflac_open_memory(buf.data(), buf.size(), nullptr);
            if (flac) {
                drflac_uint64 numFrames = flac->totalPCMFrameCount;
                uint32_t channels = flac->channels;
                sample->sampleAddr.resize(numFrames * channels * sizeof(int16_t));
                drflac_read_pcm_frames_s16(flac, numFrames, reinterpret_cast<drflac_int16*>(sample->sampleAddr.data()));
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)numFrames;
                sample->loop.count = 0xFFFFFFFFu;
                sample->mData.sampleAddr = sample->sampleAddr.data();
                sample->mData.numFrames = (uint32_t)numFrames;
                sample->mData.channels = channels;
                sample->mData.codec = 5;
                sample->mData.sampleRate = flac->sampleRate;
                sample->mData.loaded = 1;
                drflac_close(flac);
                decoded = true;
            }
        } else {
            SPDLOG_ERROR("AudioSequenceFactory: unsupported CustomFormat '{}' for seq {}", customFormat, seqId);
            return nullptr;
        }

        if (!decoded) {
            SPDLOG_ERROR("AudioSequenceFactory: failed to decode '{}' as {}", audioPath, customFormat);
            return nullptr;
        }

        AudioSequenceFactoryV0::RegisterSample((uint8_t)seqId, sample);

        // Return a sentinel so the resource manager doesn't null-ref.
        auto seq = std::make_shared<AudioSequence>(initData);
        seq->mData.id = (uint8_t)seqId;
        return seq;
    }

    // == Standard M64 + banks XML ==============================================
    std::shared_ptr<AudioSequence> seq = std::make_shared<AudioSequence>(initData);

    auto m64File =
        ShipCompat::GetResourceManager()->GetArchiveManager()->LoadFile(child->Attribute("Path"));

    tinyxml2::XMLElement* banksRoot = child->FirstChildElement("Banks");
    tinyxml2::XMLElement* banks = banksRoot->FirstChildElement();
    while (banks != nullptr) {
        auto path = banks->Attribute("Path");
        seq->banks.push_back(GameEngine::GetBankIdByName(path));
        banks = banks->NextSiblingElement();
    }

    for (size_t i = 0; i < m64File->Buffer->size(); i++) {
        seq->sampleData.push_back((uint8_t)m64File->Buffer->at(i));
    }

    seq->mData.id = child->IntAttribute("ID");
    seq->mData.banks = seq->banks.data();
    seq->mData.bankCount = seq->banks.size();
    seq->mData.data = seq->sampleData.data();
    return seq;
}
