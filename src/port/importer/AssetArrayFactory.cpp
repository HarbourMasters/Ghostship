#include "AssetArrayFactory.h"

#include "./types/AssetArray.h"
#include "spdlog/spdlog.h"
#include "ResourceUtil.h"

namespace SM64 {
std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryAssetArrayV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto asset = std::make_shared<AssetArray>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    auto count = reader->ReadUInt32();
    for (size_t i = 0; i < count; i++) {
        asset->mPtrs.push_back(reinterpret_cast<uintptr_t>(ResourceGetDataByCrc(reader->ReadUInt64())));
    }

    return asset;
}
} // namespace SM64
