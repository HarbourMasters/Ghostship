#include "MovtexQuadFactory.h"
#include "ResourceUtil.h"
#include "port/importer/types/MovtexQuad.h"

std::shared_ptr<Ship::IResource> SM64::MovtexQuadFactoryV0::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    std::shared_ptr<MovtexQuad> movtex = std::make_shared<MovtexQuad>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint32_t count = reader->ReadUInt32();

    for(size_t i = 0; i < count; i++){
        movtex->mData.push_back({
            reader->ReadInt16(),
            LoadChild<int16_t*>(reader->ReadUInt64())
        });
    }

    return movtex;
}