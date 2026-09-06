#include "Rando.h"
#include "port/ShipCompat.h"
#include <ship/Context.h>
#include "ObjectBehavior/ObjectBehavior.h"
#include "MiscBehavior/MiscBehavior.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/Rando/EntranceTracker/EntranceTracker.h"
#include "port/Rando/Spoiler/Spoiler.h"
#include "port/ShipInit.hpp"

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

int16_t selectedFileNum = 0;

// Resolved on first use rather than at load time. On Android the app directory
// comes from SDL_AndroidGetExternalStoragePath(), which calls into Java — and a
// global constructor runs at dlopen, before SDL's JNI is set up, so asking then
// aborts the process with "CallStaticObjectMethod received NULL jclass".
static const fs::path& RandomizerFolderPath() {
    static const fs::path path(Ship::Context::GetPathRelativeToAppDirectory("randomizer", "sm64"));
    return path;
}

// Entry point for the module, run once on game boot
void Rando::Init() {
    if (!fs::exists(RandomizerFolderPath())) {
        fs::create_directory(RandomizerFolderPath());
    }

    Rando::Spoiler::RefreshSpoilerLogs();
    Rando::MiscBehavior::Init();
    Rando::ObjectBehavior::Init();
    Rando::CheckTracker::Init();
    Rando::EntranceTracker::Init();
    // ShipCompat::GetFileDropMgr()->RegisterDropHandler(Rando::Spoiler::HandleFileDropped);
}

// RandoCheckId Rando::FindItemPlacement(RandoItemId randoItemId) {
//     for (auto& [randoCheckId, check] : Rando::StaticData::Checks) {
//         if (RANDO_SAVE_CHECKS[randoCheckId].randoItemId == randoItemId) {
//             return randoCheckId;
//         }
//     }
//
//     return RC_UNKNOWN;
// }
