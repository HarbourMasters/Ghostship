#include "RawTextureFactory.h"
#include "fast/resource/type/Texture.h"
#include "spdlog/spdlog.h"
#include <stb_image.h>
#include <ship/Context.h>
#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/ResourceManager.h"

namespace MK64 {
std::unordered_map<Fast::TextureType, float> TexturePixelMultipliers = {
    { Fast::TextureType::RGBA32bpp, 4.0f },           { Fast::TextureType::RGBA16bpp, 2.0f },
    { Fast::TextureType::Palette4bpp, 0.5f },         { Fast::TextureType::Palette8bpp, 1.0f },
    { Fast::TextureType::Grayscale4bpp, 0.5f },       { Fast::TextureType::Grayscale8bpp, 1.0f },
    { Fast::TextureType::GrayscaleAlpha4bpp, 0.5f },  { Fast::TextureType::GrayscaleAlpha8bpp, 1.0f },
    { Fast::TextureType::GrayscaleAlpha16bpp, 2.0f },
};

std::shared_ptr<Ship::IResource> loadPngTexture(std::shared_ptr<Ship::File> filePng,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    auto texture = std::make_shared<Fast::Texture>(initData);
    const auto res = std::static_pointer_cast<Fast::Texture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(initData->Path, true));

    int height, width = 0;
    texture->ImageData = stbi_load_from_memory((const stbi_uc*)filePng->Buffer.get()->data(),
                                               filePng->Buffer.get()->size(), &width, &height, nullptr, 4);
    texture->Width = width;
    texture->Height = height;
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    if (res != nullptr) {
        texture->HByteScale = (res->Width / texture->Width) *
                              (TexturePixelMultipliers[texture->Type] / TexturePixelMultipliers[res->Type]);
        texture->VPixelScale = res->Height / texture->Height;
    } else {
        texture->VPixelScale = 1.0f;
        texture->HByteScale = 1.0f;
    }
    return texture;
}

std::vector<std::string> extension = { ".png", ".PNG", ".jpg", ".JPG", ".jpeg", ".JPEG", ".bmp", ".BMP" };

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRawTextureV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    for (const auto& ext : extension) {
        auto filePng = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(initData->Path + ext);

        if (filePng != nullptr) {
            return loadPngTexture(filePng, initData);
        }
    }

    auto texture = std::make_shared<Fast::Texture>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    texture->Type = (Fast::TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->ImageDataSize = reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRawTextureV1::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    for (const auto& ext : extension) {
        std::shared_ptr<Ship::File> texture;

        if (initData->Path.find(ext) != std::string::npos) {
            texture = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(initData->Path);
        } else {
            texture = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(initData->Path + ext);
        }

        if (texture != nullptr) {
            return loadPngTexture(texture, initData);
        }
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
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}
} // namespace MK64