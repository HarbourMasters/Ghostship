#include "DialogFactory.h"
#include "port/importer/types/Dialog.h"
#include "spdlog/spdlog.h"

std::shared_ptr<LUS::IResource> CubeOS::DialogFactory::ReadResource(std::shared_ptr<LUS::ResourceInitData> initData, std::shared_ptr<LUS::BinaryReader> reader) {
    auto resource = std::make_shared<Dialog>(initData);
    std::shared_ptr<LUS::ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<DialogFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Dialog with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void CubeOS::DialogFactoryV0::ParseFileBinary(std::shared_ptr<LUS::BinaryReader> reader, std::shared_ptr<LUS::IResource> resource) {
    std::shared_ptr<Dialog> dialog = std::static_pointer_cast<Dialog>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, dialog);

    dialog->mData.unused = reader->ReadUInt32();
    dialog->mData.linesPerBox = reader->ReadInt8();
    dialog->mData.leftOffset = reader->ReadInt16();
    dialog->mData.width = reader->ReadInt16();

    size_t textSize = reader->ReadUInt32();
    for(size_t i = 0; i < textSize; i++){
        dialog->mText.push_back(reader->ReadUByte());
    }

    dialog->mData.str = dialog->mText.data();
}