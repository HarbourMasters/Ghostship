#include "PaintingFactory.h"
#include "ResourceUtil.h"
#include "port/importer/types/Painting.h"

std::shared_ptr<Ship::IResource> SM64::PaintingFactoryV0::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    std::shared_ptr<Painting> painting = std::make_shared<Painting>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    painting->mData.id = reader->ReadInt16();
    painting->mData.imageCount = reader->ReadInt8();
    painting->mData.textureType = reader->ReadInt8();
    painting->mData.lastFloor = reader->ReadInt8();
    painting->mData.currFloor = reader->ReadInt8();
    painting->mData.floorEntered = reader->ReadInt8();
    painting->mData.state = reader->ReadInt8();
    painting->mData.pitch = reader->ReadFloat();
    painting->mData.yaw = reader->ReadFloat();
    painting->mData.posX = reader->ReadFloat();
    painting->mData.posY = reader->ReadFloat();
    painting->mData.posZ = reader->ReadFloat();
    painting->mData.currRippleMag = reader->ReadFloat();
    painting->mData.passiveRippleMag = reader->ReadFloat();
    painting->mData.entryRippleMag = reader->ReadFloat();
    painting->mData.rippleDecay = reader->ReadFloat();
    painting->mData.passiveRippleDecay = reader->ReadFloat();
    painting->mData.entryRippleDecay = reader->ReadFloat();
    painting->mData.currRippleRate = reader->ReadFloat();
    painting->mData.passiveRippleRate = reader->ReadFloat();
    painting->mData.entryRippleRate = reader->ReadFloat();
    painting->mData.dispersionFactor = reader->ReadFloat();
    painting->mData.passiveDispersionFactor = reader->ReadFloat();
    painting->mData.entryDispersionFactor = reader->ReadFloat();
    painting->mData.rippleTimer = reader->ReadFloat();
    painting->mData.rippleX = reader->ReadFloat();
    painting->mData.rippleY = reader->ReadFloat();
    painting->mData.normalDisplayList = LoadChild<void*>(reader->ReadUInt64());
    painting->mData.textureMaps = LoadChild<int16_t*>(reader->ReadUInt64());
    painting->mData.textureArray = LoadChild<uint8_t*>(reader->ReadUInt64());
    painting->mData.textureWidth = reader->ReadInt16();
    painting->mData.textureHeight = reader->ReadInt16();
    painting->mData.rippleDisplayList = LoadChild<void*>(reader->ReadUInt64());
    painting->mData.rippleTrigger = reader->ReadInt8();
    painting->mData.alpha = reader->ReadUByte();
    painting->mData.marioWasUnder = reader->ReadInt8();
    painting->mData.marioIsUnder = reader->ReadInt8();
    painting->mData.marioWentUnder = reader->ReadInt8();
    painting->mData.size = reader->ReadFloat();

    return painting;
}