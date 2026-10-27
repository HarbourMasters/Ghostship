#pragma once

#include <ship/resource/Resource.h>
#include <ship/resource/ResourceFactoryBinary.h>
#include <ship/resource/ResourceFactoryXML.h>

#include "port/scripts/Scripts.h"

namespace SM64 {

class ScriptResource : public Ship::Resource<uintptr_t> {
  public:
    using Resource::Resource;

    uintptr_t* GetPointer() override;
    size_t GetPointerSize() override;

    Scripts::Built mBuilt;
};

// Binary scripts, as Torch writes them:
//   u32 array count, string entry array (empty for the default)
//   per array: string name, u32 command count
//   per command: string name, u32 source line, u8 argument count
//   per argument: u8 type (0 number, 1 symbol, 2 path), then an s64 for numbers or a string otherwise
class ScriptFactoryV0 : public Ship::ResourceFactoryBinary {
  public:
    explicit ScriptFactoryV0(Scripts::Kind kind) : mKind(kind) {
    }
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;

  private:
    Scripts::Kind mKind;
};

// Text scripts: <LevelScript Version="0" Entry="name">decomp-style source</LevelScript>, likewise <BehaviorScript>.
class ScriptXMLFactoryV0 : public Ship::ResourceFactoryXML {
  public:
    explicit ScriptXMLFactoryV0(Scripts::Kind kind) : mKind(kind) {
    }
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;

  private:
    Scripts::Kind mKind;
};

} // namespace SM64
