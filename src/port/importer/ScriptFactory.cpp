#include "ScriptFactory.h"

#include <spdlog/spdlog.h>
#include <tinyxml2.h>

namespace SM64 {

uintptr_t* ScriptResource::GetPointer() {
    return const_cast<uintptr_t*>(mBuilt.Entry());
}

size_t ScriptResource::GetPointerSize() {
    return mBuilt.arrays.empty() ? 0 : mBuilt.arrays[mBuilt.entry].size() * sizeof(uintptr_t);
}

static std::shared_ptr<ScriptResource> BuildResource(const Scripts::Program& program,
                                                     std::shared_ptr<Ship::ResourceInitData> initData) {
    auto resource = std::make_shared<ScriptResource>(initData);
    std::string error;
    if (!Scripts::Build(program, resource->mBuilt, error)) {
        SPDLOG_ERROR("Script {}: {}", initData->Path, error);
        return nullptr;
    }
    return resource;
}

std::shared_ptr<Ship::IResource> ScriptFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                               std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    Scripts::Program program;
    program.kind = mKind;
    const uint32_t arrayCount = reader->ReadUInt32();
    program.entry = reader->ReadString();
    program.arrays.resize(arrayCount);
    for (Scripts::Array& array : program.arrays) {
        array.name = reader->ReadString();
        array.commands.resize(reader->ReadUInt32());
        for (Scripts::Command& cmd : array.commands) {
            cmd.name = reader->ReadString();
            cmd.line = reader->ReadUInt32();
            cmd.args.resize(reader->ReadUByte());
            for (Scripts::Value& value : cmd.args) {
                value.type = static_cast<Scripts::Value::Type>(reader->ReadUByte());
                if (value.type == Scripts::Value::Type::Number) {
                    value.number = reader->ReadInt64();
                } else {
                    value.text = reader->ReadString();
                }
            }
        }
    }
    return BuildResource(program, initData);
}

std::shared_ptr<Ship::IResource> ScriptXMLFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                                  std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }
    auto root = std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement();
    const char* text = root->GetText();

    std::string error;
    auto program = Scripts::ParseText(mKind, text != nullptr ? text : "", error);
    if (!program) {
        SPDLOG_ERROR("Script {}: {}", initData->Path, error);
        return nullptr;
    }
    if (const char* entry = root->Attribute("Entry")) {
        program->entry = entry;
    }
    return BuildResource(*program, initData);
}

} // namespace SM64
