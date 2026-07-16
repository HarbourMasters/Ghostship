#pragma once

#include <bridge/resourcebridge.h>
#include "port/ShipCompat.h"
#include <ship/resource/ResourceManager.h>
#include <ship/Context.h>

namespace SM64 {
template <typename T> T LoadChild(uint64_t crc) {
    auto path = ResourceGetNameByCrc(crc);
    if (path == nullptr) {
        return nullptr;
    }
    printf("LoadChild: %s\n", path);
    auto asset = ShipCompat::GetResourceManager()->LoadResourceProcess(path);
    return asset ? static_cast<T>(asset->GetRawPointer()) : nullptr;
}
template <typename T> T LoadChild(const char* path) {
    if (path == nullptr) {
        return nullptr;
    }
    auto asset = ShipCompat::GetResourceManager()->LoadResourceProcess(path);
    return asset ? static_cast<T>(asset->GetRawPointer()) : nullptr;
}
}
