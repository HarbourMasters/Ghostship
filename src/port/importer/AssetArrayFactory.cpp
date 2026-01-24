#include "AssetArrayFactory.h"

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
        auto path = ResourceGetNameByCrc(reader->ReadUInt64());
        if (path == nullptr) {
            array->mPtrs.push_back(0);
            continue;
        }
        auto asset = Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(path);
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
