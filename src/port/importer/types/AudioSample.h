#pragma once

#include <cstdint>
#include <ship/resource/Resource.h>


struct AdpcmLoop {
    uint32_t start;
    uint32_t end;
    uint32_t count;
    uint32_t pad;
    int16_t* state; // only exists if count != 0. 8-byte aligned
};

struct AdpcmBook {
    int32_t order;
    int32_t npredictors;
    int16_t* book; // size 8 * order * npredictors. 8-byte aligned
};

struct AudioBankSample {
    uint8_t unused;
    uint8_t loaded;
    uint8_t *sampleAddr;
    AdpcmLoop *loop;
    AdpcmBook *book;
    uint32_t sampleSize; // never read. either 0 or 1 mod 9, depending on padding
    // Fields below mirror the C struct layout in internal.h (non-SH), then extend it.
    // codec and numFrames must stay in this order and position — C synthesis code reads them.
    uint8_t codec = 0;      // CODEC_ADPCM=0, CODEC_S8=1, CODEC_SKIP=2, CODEC_S16=5
    uint32_t numFrames = 0; // total PCM sample frames; used by CODEC_S16 path
    // C++ only — not accessed from C synthesis code, safe to append after numFrames.
    uint32_t channels = 1;
    uint32_t sampleRate = 32000;
};

namespace SM64 {


class AudioSample : public Ship::Resource<AudioBankSample> {
  public:
    using Resource::Resource;

    AudioSample() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    AudioBankSample* GetPointer();
    size_t GetPointerSize();

    AudioBankSample mData;

    AdpcmLoop loop;
    AdpcmBook book;
    std::vector<uint8_t> sampleAddr;
};
}