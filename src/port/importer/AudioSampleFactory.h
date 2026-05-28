#pragma once

#include <ship/resource/Resource.h>
#include <ship/resource/ResourceFactoryBinary.h>
#include <ship/resource/ResourceFactoryXML.h>
#include <memory>
#include <string>

namespace SM64 {
class AudioSample;

class AudioSampleFactoryV0 : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;

    static std::shared_ptr<AudioSample> FromMp3(const std::string& path, bool loop = true);
};

class AudioSampleXMLFactoryV0 : public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}
