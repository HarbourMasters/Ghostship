#include "AnimationFactory.h"
#include "port/importer/types/Animation.h"
#include "spdlog/spdlog.h"

std::shared_ptr<LUS::IResource> CubeOS::AnimationFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<Animation>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<AnimationFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Animation with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void CubeOS::AnimationFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<Animation> animation = std::static_pointer_cast<Animation>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, animation);

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

    animation->mData.index = (uint16_t*)animation->indices.data();
    animation->mData.values = (int16_t*)animation->values.data();
}