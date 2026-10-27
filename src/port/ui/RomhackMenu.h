#pragma once

#include <string>
#include <vector>

#include "port/romhack/Romhack.h"

// Generating romhacks for the Mods menu, which lists them with the base game choices (ModManager).
namespace RomhackMenu {

// The hacks this build has recipes for, read once.
const std::vector<Romhack::Supported>& Supported();
// Asks for a ROM and extracts the hack from it in the background: the base ROM for a hack whose package carries
// its patch, or a patched ROM when hack is null.
void Generate(const Romhack::Supported* hack);
// Shows the result of a finished extraction. Call every frame the menu is drawn.
void Update();
// While extracting: the progress bar, what it is working on, and the warnings and errors so far.
void DrawProgress();

bool Extracting();
float Progress();
// True once after a romhack was generated, so the list of installed ones is read again.
bool TakeInstalledChanged();

} // namespace RomhackMenu
