#pragma once

#include <ship/resource/Resource.h>
#include <ship/resource/ResourceFactoryXML.h>
#include <ship/resource/ResourceFactoryBinary.h>
#include <memory>
#include <cstdint>

namespace SM64 {
class AudioSample;

class AudioSequenceFactoryV0 : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;

    static void RegisterSample(uint8_t seqId, std::shared_ptr<AudioSample> sample);
};

class AudioSequenceXMLFactoryV0 : public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}
