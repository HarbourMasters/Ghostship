#pragma once

#include <ship/resource/Resource.h>
#include <ship/resource/ResourceFactoryXML.h>
#include <ship/resource/ResourceFactoryBinary.h>
#include <memory>
#include <cstdint>

struct AudioBankSound;
struct AudioSequenceData;
struct CtlEntry;

namespace SM64 {
class AudioSample;

class AudioSequenceFactoryV0 : public Ship::ResourceFactoryBinary {
  public:
    static constexpr uint8_t kStreamedBankId = 200;

    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;

    static void RegisterSample(uint8_t seqId, std::shared_ptr<AudioSample> sample);

    // Lock-free; safe to call from the audio thread.
    static AudioBankSound*    GetStreamedSound(uint8_t seqId, uint8_t channelIdx);
    static AudioSequenceData* GetStreamedSeqData(uint8_t seqId);
};

class AudioSequenceXMLFactoryV0 : public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}
