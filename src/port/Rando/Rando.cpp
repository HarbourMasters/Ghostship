#include "Rando.h"
#include <ship/Context.h>
#include "port/mods/PortEnhancements.h"
// #include "2s2h/GameInteractor/GameInteractor.h"
#include "ObjectBehavior/ObjectBehavior.h"
#include "MiscBehavior/MiscBehavior.h"
// #include "Rando/Spoiler/Spoiler.h"
// #include "Rando/CheckTracker/CheckTracker.h"
#include "port/ShipInit.hpp"

int16_t selectedFileNum = 0;

// When a save is loaded, we want to unregister all hooks and re-register them if it's a rando save
// void OnSaveLoadHandler(s16 fileNum) {
//     Rando::MiscBehavior::OnFileLoad();
//     Rando::ActorBehavior::OnFileLoad();
//     Rando::CheckTracker::OnFileLoad();
//
//     // Re-initalizes enhancements that are effected by the save being rando or not
//     ShipInit::Init("IS_RANDO");
// }

// Entry point for the module, run once on game boot
void Rando::Init() {
    // Rando::Spoiler::RefreshOptions();
    Rando::MiscBehavior::Init();
    Rando::ObjectBehavior::Init();
    // Rando::CheckTracker::Init();
    // Ship::Context::GetInstance()->GetFileDropMgr()->RegisterDropHandler(Rando::Spoiler::HandleFileDropped);

    // GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>(OnSaveLoadHandler);
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
