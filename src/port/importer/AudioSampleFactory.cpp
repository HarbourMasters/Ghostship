#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#include "AudioSampleFactory.h"
#include "port/ShipCompat.h"
#include <iostream>
#include <thread>
#include <vector>

#ifdef __SWITCH__
#define fopen64 fopen
#endif

#include <dr_mp3.h>
#include <dr_wav.h>
#include <dr_flac.h>
#include <tinyxml2.h>

#if __has_include(<stb_vorbis.h>)
#define STB_VORBIS_IMPLEMENTATION
#include <stb_vorbis.h>
#define GS_HAS_STB_VORBIS
#endif

#include <spdlog/spdlog.h>
#include <ship/core/Context.h>
#include <ship/resource/ResourceManager.h>

#include "macros.h"
#include "port/importer/types/AudioSample.h"

// ── Async MP3 worker ─────────────────────────────────────────────────────────

static void Mp3DecoderWorker(std::shared_ptr<SM64::AudioSample> sample, std::vector<uint8_t> buffer) {
    drmp3 mp3;
    if (!drmp3_init_memory(&mp3, buffer.data(), buffer.size(), nullptr))
        return;

    drmp3_uint64 numFrames = drmp3_get_pcm_frame_count(&mp3);
    uint32_t channels = mp3.channels;

    sample->sampleAddr.resize(numFrames * channels * sizeof(int16_t));
    drmp3_read_pcm_frames_s16(&mp3, numFrames, reinterpret_cast<drmp3_int16*>(sample->sampleAddr.data()));
    drmp3_uninit(&mp3);

    sample->mData.sampleAddr = sample->sampleAddr.data();
    sample->mData.numFrames = (uint32_t)numFrames;
    sample->mData.channels = channels;
    sample->mData.codec = 5; // CODEC_S16
    sample->mData.loaded = 1;
}

// ── FromMp3: filesystem path → AudioSample (synchronous) ─────────────────────

std::shared_ptr<SM64::AudioSample> SM64::AudioSampleFactoryV0::FromMp3(const std::string& path, bool loop) {
    drmp3_config cfg{};
    drmp3_uint64 totalFrames = 0;
    drmp3_int16* raw = drmp3_open_file_and_read_pcm_frames_s16(path.c_str(), &cfg, &totalFrames, nullptr);
    if (!raw || totalFrames == 0) {
        SPDLOG_ERROR("AudioSampleFactory: failed to decode '{}'", path);
        if (raw)
            drmp3_free(raw, nullptr);
        return nullptr;
    }

    auto sample = std::make_shared<SM64::AudioSample>();
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
    sample->loop.count = loop ? 0xFFFFFFFFu : 0u;
    sample->loop.state = nullptr;

    sample->book = { 0, 0, nullptr };

    sample->mData.unused = 0;
    sample->mData.loaded = 1;
    sample->mData.sampleAddr = sample->sampleAddr.data();
    sample->mData.loop = &sample->loop;
    sample->mData.book = &sample->book;
    sample->mData.sampleSize = 0;
    sample->mData.codec = 5; // CODEC_S16
    sample->mData.numFrames = (uint32_t)totalFrames;
    sample->mData.channels = 1;
    sample->mData.sampleRate = cfg.sampleRate;

    return sample;
}

static void WavDecoderWorker(std::shared_ptr<SM64::AudioSample> sample, std::vector<uint8_t> buffer) {
    drwav wav;
    if (!drwav_init_memory(&wav, buffer.data(), buffer.size(), nullptr))
        return;

    drwav_uint64 numFrames = wav.totalPCMFrameCount;
    uint32_t channels = wav.channels;

    sample->sampleAddr.resize(numFrames * channels * sizeof(int16_t));
    drwav_read_pcm_frames_s16(&wav, numFrames, reinterpret_cast<drwav_int16*>(sample->sampleAddr.data()));
    drwav_uninit(&wav);

    sample->mData.sampleAddr = sample->sampleAddr.data();
    sample->mData.numFrames = (uint32_t)numFrames;
    sample->mData.channels = channels;
    sample->mData.codec = 5; // CODEC_S16
    sample->mData.loaded = 1;
}

static void FlacDecoderWorker(std::shared_ptr<SM64::AudioSample> sample, std::vector<uint8_t> buffer) {
    drflac* flac = drflac_open_memory(buffer.data(), buffer.size(), nullptr);
    if (!flac)
        return;

    drflac_uint64 numFrames = flac->totalPCMFrameCount;
    uint32_t channels = flac->channels;

    sample->sampleAddr.resize(numFrames * channels * sizeof(int16_t));
    drflac_read_pcm_frames_s16(flac, numFrames, reinterpret_cast<drflac_int16*>(sample->sampleAddr.data()));
    drflac_close(flac);

    sample->mData.sampleAddr = sample->sampleAddr.data();
    sample->mData.numFrames = (uint32_t)numFrames;
    sample->mData.channels = channels;
    sample->mData.codec = 5; // CODEC_S16
    sample->mData.loaded = 1;
}

#ifdef GS_HAS_STB_VORBIS
static void OggDecoderWorker(std::shared_ptr<SM64::AudioSample> sample, std::vector<uint8_t> buffer) {
    int channels = 0, sampleRate = 0;
    short* output = nullptr;
    int numFrames = stb_vorbis_decode_memory(buffer.data(), (int)buffer.size(), &channels, &sampleRate, &output);
    if (numFrames < 0 || !output)
        return;

    sample->sampleAddr.resize((size_t)numFrames * channels * sizeof(int16_t));
    memcpy(sample->sampleAddr.data(), output, sample->sampleAddr.size());
    free(output);

    sample->mData.sampleRate = (uint32_t)sampleRate;
    sample->mData.sampleAddr = sample->sampleAddr.data();
    sample->mData.numFrames = (uint32_t)numFrames;
    sample->mData.channels = (uint32_t)channels;
    sample->mData.codec = 5; // CODEC_S16
    sample->mData.loaded = 1;
}
#endif

std::shared_ptr<Ship::IResource>
SM64::AudioSampleFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                         std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<AudioSample> bank = std::make_shared<AudioSample>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    bank->loop.start = reader->ReadUInt32();
    bank->loop.end = reader->ReadUInt32();
    bank->loop.count = reader->ReadInt32();
    bank->loop.pad = reader->ReadInt32();

    uint32_t stateSize = reader->ReadUInt32();
    if (stateSize > 0) {
        bank->loop.state = new int16_t[stateSize];
        reader->Read((char*)bank->loop.state, stateSize * sizeof(int16_t));
    } else {
        bank->loop.state = nullptr;
    }

    bank->book.order = reader->ReadInt32();
    bank->book.npredictors = reader->ReadInt32();

    uint32_t tableSize = reader->ReadUInt32();
    bank->book.book = new int16_t[tableSize];
    reader->Read((char*)bank->book.book, tableSize * sizeof(int16_t));

    int32_t sampleSize = reader->ReadInt32();
    char* sampleData = new char[ROUND_UP_8(sampleSize)];
    reader->Read(sampleData, sampleSize);

    bank->mData.unused = 0;
    bank->mData.loaded = 1;
    bank->mData.loop = &bank->loop;
    bank->mData.book = &bank->book;
    bank->mData.sampleAddr = (uint8_t*)sampleData;
    bank->mData.sampleSize = sampleSize;

    return bank;
}

// Mirrors AudioContext::GetCodecStr() in reverse (Starship v1 Torch).
static uint8_t CodecFromStr(const char* str) {
    if (!str)
        return 0; // CODEC_ADPCM
    if (strcmp(str, "S8") == 0)
        return 1;
    if (strcmp(str, "SKIP") == 0)
        return 2;
    if (strcmp(str, "HALF") == 0)
        return 3;
    if (strcmp(str, "ADPCM_HALF") == 0)
        return 4;
    if (strcmp(str, "S16") == 0)
        return 5;
    return 0;
}

std::shared_ptr<Ship::IResource>
SM64::AudioSampleXMLFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto sample = std::make_shared<SM64::AudioSample>(initData);
    auto child = std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement();

    const char* customFormat = child->Attribute("CustomFormat");
    const char* path = child->Attribute("Path");

    sample->loop.state = nullptr;
    sample->book = { 0, 0, nullptr };
    sample->mData.loop = &sample->loop;
    sample->mData.book = &sample->book;
    sample->mData.loaded = 0;

    // Starship v1 attributes — read unconditionally so they apply to both paths.
    sample->mData.codec = CodecFromStr(child->Attribute("Codec"));
    sample->mData.unused = (uint8_t)child->IntAttribute("bit26", 0);
    float tuning = child->FloatAttribute("Tuning", 0.0f);
    if (tuning > 0.0f) {
        sample->mData.sampleRate = (uint32_t)(tuning * 32000.0f);
    }

    if (customFormat != nullptr && path != nullptr) {
        auto audioFile = ShipCompat::GetResourceManager()->GetArchiveManager()->LoadFile(path);
        if (!audioFile || !audioFile->Buffer) {
            SPDLOG_ERROR("AudioSampleFactory: could not load '{}'", path);
            return nullptr;
        }

        std::vector<uint8_t> buf(audioFile->Buffer->begin(), audioFile->Buffer->end());

        if (strcmp(customFormat, "mp3") == 0) {
            drmp3 mp3_hdr;
            if (drmp3_init_memory(&mp3_hdr, buf.data(), buf.size(), nullptr)) {
                sample->mData.sampleRate = mp3_hdr.sampleRate;
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)drmp3_get_pcm_frame_count(&mp3_hdr);
                sample->loop.count = 0;
                drmp3_uninit(&mp3_hdr);
            }
            std::thread(Mp3DecoderWorker, sample, std::move(buf)).detach();
            return sample;
        }

        if (strcmp(customFormat, "wav") == 0 || strcmp(customFormat, "aiff") == 0) {
            drwav wav_hdr;
            if (drwav_init_memory(&wav_hdr, buf.data(), buf.size(), nullptr)) {
                sample->mData.sampleRate = wav_hdr.sampleRate;
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)wav_hdr.totalPCMFrameCount;
                sample->loop.count = 0;
                drwav_uninit(&wav_hdr);
            }
            std::thread(WavDecoderWorker, sample, std::move(buf)).detach();
            return sample;
        }

        if (strcmp(customFormat, "flac") == 0) {
            drflac* flac_hdr = drflac_open_memory(buf.data(), buf.size(), nullptr);
            if (flac_hdr) {
                sample->mData.sampleRate = flac_hdr->sampleRate;
                sample->loop.start = 0;
                sample->loop.end = (uint32_t)flac_hdr->totalPCMFrameCount;
                sample->loop.count = 0;
                drflac_close(flac_hdr);
            }
            std::thread(FlacDecoderWorker, sample, std::move(buf)).detach();
            return sample;
        }

#ifdef GS_HAS_STB_VORBIS
        if (strcmp(customFormat, "ogg") == 0) {
            int error = 0;
            stb_vorbis* v = stb_vorbis_open_memory(buf.data(), (int)buf.size(), &error, nullptr);
            if (v) {
                stb_vorbis_info info = stb_vorbis_get_info(v);
                sample->mData.sampleRate = info.sample_rate;
                sample->loop.start = 0;
                sample->loop.end = stb_vorbis_stream_length_in_samples(v);
                sample->loop.count = 0;
                stb_vorbis_close(v);
            }
            std::thread(OggDecoderWorker, sample, std::move(buf)).detach();
            return sample;
        }
#endif

        SPDLOG_ERROR("AudioSampleFactory: unsupported CustomFormat '{}'", customFormat);
        return nullptr;
    }

    // ADPCM / S16 path — loop, book, and raw sample data from archive.
    tinyxml2::XMLElement* loopRoot = child->FirstChildElement("ADPCMLoop");
    if (loopRoot != nullptr) {
        sample->loop.start = loopRoot->UnsignedAttribute("Start");
        sample->loop.end = loopRoot->UnsignedAttribute("End");
        sample->loop.count = loopRoot->UnsignedAttribute("Count");

        // Starship v1: predictor state entries serialised as <Predictor State="N"/>.
        // Only present when Count != 0 (looping sample with ADPCM state restore).
        if (sample->loop.count != 0) {
            std::vector<int16_t> states;
            for (auto* e = loopRoot->FirstChildElement("Predictor"); e != nullptr;
                 e = e->NextSiblingElement("Predictor")) {
                states.push_back((int16_t)e->IntAttribute("State"));
            }
            if (!states.empty()) {
                sample->loop.state = new int16_t[states.size()];
                memcpy(sample->loop.state, states.data(), states.size() * sizeof(int16_t));
            }
        }
    }

    tinyxml2::XMLElement* bookRoot = child->FirstChildElement("ADPCMBook");
    if (bookRoot != nullptr) {
        sample->book.order = bookRoot->IntAttribute("Order");
        sample->book.npredictors = bookRoot->IntAttribute("Npredictors");
        size_t numBooks = (size_t)sample->book.order * sample->book.npredictors * 8;
        sample->book.book = new int16_t[numBooks];
        size_t i = 0;
        for (auto* e = bookRoot->FirstChildElement("Book"); e != nullptr; e = e->NextSiblingElement())
            sample->book.book[i++] = e->IntAttribute("Page");
    }

    if (path != nullptr) {
        auto audioFile = ShipCompat::GetResourceManager()->GetArchiveManager()->LoadFile(path);
        if (audioFile && audioFile->Buffer) {
            uint32_t size = (uint32_t)audioFile->Buffer->size();
            char* data = new char[ROUND_UP_8(size)];
            memcpy(data, audioFile->Buffer->data(), size);
            sample->mData.sampleAddr = (uint8_t*)data;
            sample->mData.sampleSize = size;
        }
    }

    sample->mData.loaded = 1;
    return sample;
}
