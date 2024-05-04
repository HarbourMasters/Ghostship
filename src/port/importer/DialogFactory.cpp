#include "DialogFactory.h"
#include "port/importer/types/Dialog.h"
#include "spdlog/spdlog.h"

std::shared_ptr<LUS::IResource> SM64::DialogFactoryV0::ReadResource(std::shared_ptr<LUS::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    std::shared_ptr<Dialog> dialog = std::make_shared<Dialog>(file->InitData);
    auto reader = std::get<std::shared_ptr<LUS::BinaryReader>>(file->Reader);

    dialog->mData.unused = reader->ReadUInt32();
    dialog->mData.linesPerBox = reader->ReadInt8();
    dialog->mData.leftOffset = reader->ReadInt16();
    dialog->mData.width = reader->ReadInt16();

    size_t textSize = reader->ReadUInt32();
    for(size_t i = 0; i < textSize; i++){
        dialog->mText.push_back(reader->ReadUByte());
    }

    dialog->mData.str = dialog->mText.data();
    return dialog;
}