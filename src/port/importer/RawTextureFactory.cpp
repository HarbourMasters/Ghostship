#include "RawTextureFactory.h"
#include "fast/resource/type/Texture.h"
#include "spdlog/spdlog.h"
#include <stb_image.h>

namespace MK64 {

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRawTextureV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto texture = std::make_shared<Fast::Texture>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    texture->Type = (Fast::TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->ImageDataSize = reader->ReadUInt32();

    std::vector<uint8_t> buf(texture->ImageDataSize);
    reader->Read(reinterpret_cast<char*>(buf.data()), texture->ImageDataSize);

    int w, h;
    texture->ImageData = stbi_load_from_memory(buf.data(), static_cast<int>(buf.size()), &w, &h, nullptr, 4);

    if (!texture->ImageData) {
        SPDLOG_ERROR("RawTextureFactory V0: stbi failed for {}: {}", initData->Identifier.GetPath(),
                     stbi_failure_reason());
        return nullptr;
    }

    texture->Width = static_cast<uint32_t>(w);
    texture->Height = static_cast<uint32_t>(h);
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    texture->HByteScale = 1.0f;
    texture->VPixelScale = 1.0f;

    return texture;
}

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRawTextureV1::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto texture = std::make_shared<Fast::Texture>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    texture->Type = (Fast::TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->Flags = reader->ReadUInt32();
    texture->HByteScale = reader->ReadFloat();
    texture->VPixelScale = reader->ReadFloat();
    texture->ImageDataSize = reader->ReadUInt32();

    std::vector<uint8_t> buf(texture->ImageDataSize);
    reader->Read(reinterpret_cast<char*>(buf.data()), texture->ImageDataSize);

    int w, h;
    texture->ImageData = stbi_load_from_memory(buf.data(), static_cast<int>(buf.size()), &w, &h, nullptr, 4);

    if (!texture->ImageData) {
        SPDLOG_ERROR("RawTextureFactory V1: stbi failed for {}: {}", initData->Identifier.GetPath(),
                     stbi_failure_reason());
        return nullptr;
    }

    texture->Width = static_cast<uint32_t>(w);
    texture->Height = static_cast<uint32_t>(h);
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    texture->HByteScale = 1.0f;
    texture->VPixelScale = 1.0f;

    return texture;
}

} // namespace MK64
