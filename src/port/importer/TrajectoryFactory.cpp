#include "TrajectoryFactory.h"
#include "port/importer/types/Trajectory.h"

std::shared_ptr<Ship::IResource>
SM64::TrajectoryFactoryV0::ReadResource(std::shared_ptr<Ship::File> file,
                                        std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<TrajectoryResource> trajectory = std::make_shared<TrajectoryResource>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint32_t count = reader->ReadUInt32();

    for (size_t i = 0; i < count; i++) {
        trajectory->mData.push_back(
            { reader->ReadInt16(), reader->ReadInt16(), reader->ReadInt16(), reader->ReadInt16() });
    }

    return trajectory;
}