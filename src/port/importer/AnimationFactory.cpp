#include "AnimationFactory.h"
#include "port/importer/types/Animation.h"
#include "spdlog/spdlog.h"

std::shared_ptr<Ship::IResource>
SM64::AnimationFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                       std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<Animation> animation = std::make_shared<Animation>(initData);
    const auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    animation->mData.flags = reader->ReadInt16();
    animation->mData.animYTransDivisor = reader->ReadInt16();
    animation->mData.startFrame = reader->ReadInt16();
    animation->mData.loopStart = reader->ReadInt16();
    animation->mData.loopEnd = reader->ReadInt16();
    animation->mData.unusedBoneCount = reader->ReadInt16();
    animation->mData.length = reader->ReadUInt64();

    uint32_t indexCount = reader->ReadUInt32();
    animation->indices.reserve(indexCount);
    for (uint32_t i = 0; i < indexCount; i++) {
        animation->indices.push_back(reader->ReadUInt16());
    }

    uint32_t valuesCount = reader->ReadUInt32();
    animation->values.reserve(valuesCount);
    for (uint32_t i = 0; i < valuesCount; i++) {
        animation->values.push_back(reader->ReadInt16());
    }

    animation->mData.index = animation->indices.data();
    animation->mData.values = animation->values.data();

    return animation;
}