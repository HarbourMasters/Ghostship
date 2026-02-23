#pragma once

#include <ship/resource/Resource.h>
#include <ship/resource/ResourceFactoryXML.h>
#include <ship/resource/ResourceFactoryBinary.h>

namespace SM64 {
class AudioSequenceFactoryV0 : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};

class AudioSequenceXMLFactoryV0 : public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}
