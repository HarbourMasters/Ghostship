#include "AssetArrayFactory.h"
#include "port/ShipCompat.h"

#include "./types/AssetArray.h"
#include "spdlog/spdlog.h"
#include "ResourceUtil.h"
#include <fast/resource/ResourceType.h>

namespace SM64 {
std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryAssetArrayV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto array = std::make_shared<AssetArray>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    auto count = reader->ReadUInt32();
    for (size_t i = 0; i < count; i++) {
        auto crc = reader->ReadUInt64();
        auto path = ResourceGetNameByCrc(crc);
        if (path == nullptr) {
            // A zero crc is a null entry in the original table — tables like
            // main_hud_lut are indexed by character code and are full of gaps.
            // A non-zero crc that resolves to nothing is a real missing asset,
            // and it ends up as a null pointer the renderer cannot see coming.
            if (crc != 0) {
                SPDLOG_ERROR("AssetArray {}: entry {} of {} has no asset for crc {:#x}", initData->Path, i, count, crc);
            }
            array->mPtrs.push_back(0);
            continue;
        }
        auto asset = ShipCompat::GetResourceManager()->LoadResourceProcess(path);
        if (asset == nullptr) {
            SPDLOG_ERROR("AssetArray {}: entry {} failed to load {}", initData->Path, i, path);
        }
        if (asset != nullptr) {
            auto data = asset->GetInitData();
            if (data->Type == (uint32_t)Fast::ResourceType::Texture) {
                array->mPaths.push_back("__OTR__" + data->Path);
                array->mPtrs.push_back(reinterpret_cast<uintptr_t>(array->mPaths.back().c_str()));
            } else {
                array->mPtrs.push_back(reinterpret_cast<uintptr_t>(asset->GetRawPointer()));
            }
        } else {
            array->mPtrs.push_back(0);
        }
    }

    return array;
}
} // namespace SM64
