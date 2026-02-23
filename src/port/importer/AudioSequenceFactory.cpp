#include "AudioSequenceFactory.h"
#include "port/importer/types/AudioSequence.h"
#include "spdlog/spdlog.h"
#include "port/Engine.h"
#include <tinyxml2.h>
#include "port/importer/types/AudioBank.h"

std::shared_ptr<Ship::IResource>
SM64::AudioSequenceFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                           std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<AudioSequence> bank = std::make_shared<AudioSequence>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint8_t id = reader->ReadUInt32();
    size_t bankCount = reader->ReadUInt32();
    for (size_t i = 0; i < bankCount; i++) {
        std::string bankName = reader->ReadString();
        bank->banks.push_back(GameEngine::GetBankIdByName(bankName));
    }

    size_t sampleSize = reader->ReadUInt32();
    for (size_t i = 0; i < sampleSize; i++) {
        bank->sampleData.push_back(reader->ReadUByte());
    }

    bank->mData.bankCount = bankCount;
    bank->mData.banks = bank->banks.data();
    bank->mData.data = bank->sampleData.data();
    bank->mData.id = id;

    return bank;
}


std::shared_ptr<Ship::IResource>
SM64::AudioSequenceXMLFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                           std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<AudioSequence> seq = std::make_shared<AudioSequence>(initData);
    auto child =
        std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement();

    auto m64File = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->LoadFile(child->Attribute("Path"));

    tinyxml2::XMLElement* banksRoot = child->FirstChildElement("Banks");
    tinyxml2::XMLElement* banks = banksRoot->FirstChildElement();
    while (banks != nullptr) {
        auto path = banks->Attribute("Path");
        seq->banks.push_back(GameEngine::GetBankIdByName(path));
        banks = banks->NextSiblingElement();
    }

    // Copy m64 to sampleData
    for(size_t i = 0; i < m64File->Buffer->size(); i++) {
        seq->sampleData.push_back((uint8_t) m64File->Buffer->at(i));
    }

    seq->mData.id = child->IntAttribute("ID");
    seq->mData.banks = seq->banks.data();
    seq->mData.bankCount = seq->banks.size();
    seq->mData.data = seq->sampleData.data();
    return seq;
}
