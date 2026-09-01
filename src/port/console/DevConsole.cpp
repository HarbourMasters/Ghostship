#include "DevConsole.h"
#include "port/ShipCompat.h"

#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/ConsoleWindow.h>
#include <ship/debug/Console.h>

#define CMD_REGISTER ShipCompat::GetConsole()->AddCommand
// TODO: Commands should be using the output passed in.
#define ERROR_MESSAGE                                                                                              \
    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(ShipCompat::GetWindow()->GetGui()->GetGuiWindow("Console")) \
        ->SendErrorMessage
#define INFO_MESSAGE                                                                                               \
    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(ShipCompat::GetWindow()->GetGui()->GetGuiWindow("Console")) \
        ->SendInfoMessage

extern "C" {
void warp_special(int32_t arg);
extern int16_t sCurrPlayMode; // level_update.h
}

static bool ResetHandler(std::shared_ptr<Ship::Console> Console, std::vector<std::string> args, std::string* output) {
    // Do not reset unless currently playing a level
    if (sCurrPlayMode != 0) { // PLAY_MODE_NORMAL
        ERROR_MESSAGE("sCurrPlayMode != PLAY_MODE_NORMAL");
        return 1;
    }

    warp_special(-8);
    return 0;
}

void DevConsole_Init(void) {
    CMD_REGISTER("reset", { ResetHandler, "Resets the game." });
}