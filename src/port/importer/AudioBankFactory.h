#pragma once

#include "ResourceFactory.h"

namespace CubeOS{
class AudioBankFactory : public LUS::ResourceFactory {
  public:
    std::shared_ptr<LUS::IResource>
    ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) override;
};

class AudioBankFactoryV0 : public LUS::ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) override;
};
}
