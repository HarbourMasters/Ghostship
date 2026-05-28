#include "TextFactory.h"
#include "port/importer/types/Text.h"

namespace SM64 {
std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryTextV0::ReadResource(std::shared_ptr<Ship::File> file,
                                          std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto text = std::make_shared<Text>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    text->Data = reader->ReadCString();
    return text;
}
} // namespace SM64