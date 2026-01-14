#include "MiscBehavior.h"

// Entry point for the module, run once on game boot
void Rando::MiscBehavior::Init() {
    Rando::MiscBehavior::OnFileLoad();
    Rando::MiscBehavior::OnFileSave();
}

