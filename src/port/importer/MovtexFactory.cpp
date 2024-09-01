#include "MovtexFactory.h"
#include "port/importer/types/Movtex.h"

std::shared_ptr<Ship::IResource> SM64::MovtexFactoryV0::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    std::shared_ptr<Movtex> movtex = std::make_shared<Movtex>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint32_t count = reader->ReadUInt32();

    for(size_t i = 0; i < count; i++){
        movtex->mData.push_back(reader->ReadInt16());
    }

    return movtex;
}