#include "MacroObjectFactory.h"
#include "port/importer/types/MacroObject.h"

#define MACRO_OBJECT_END() 0x001E

std::shared_ptr<Ship::IResource>
SM64::MacroObjectFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                         std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<MacroObject> macro = std::make_shared<MacroObject>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint32_t count = reader->ReadUInt32();

    for (size_t i = 0; i < count; i++) {
        macro->mData.push_back(reader->ReadInt16());
        macro->mData.push_back(reader->ReadInt16());
        macro->mData.push_back(reader->ReadInt16());
        macro->mData.push_back(reader->ReadInt16());
        macro->mData.push_back(reader->ReadInt16());
        macro->mData.push_back(reader->ReadInt16());
    }

    macro->mData.push_back(MACRO_OBJECT_END());
    return macro;
}