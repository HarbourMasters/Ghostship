#include "DevConsole.h"

#include <ship/Context.h>
#include <ship/window/Window.h>

#define CMD_REGISTER Ship::Context::GetInstance()->GetConsole()->AddCommand
// TODO: Commands should be using the output passed in.
#define ERROR_MESSAGE                                                                 \
    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(                               \
        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console")) \
        ->SendErrorMessage
#define INFO_MESSAGE                                                                  \
    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(                               \
        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console")) \
        ->SendInfoMessage

extern "C" {
void handle_nmi_request(void);
}

static bool ResetHandler(std::shared_ptr<Ship::Console> Console, std::vector<std::string> args, std::string* output) {
    handle_nmi_request();
    return 0;
}

void DevConsole_Init(void) {
    CMD_REGISTER("reset", { ResetHandler, "Resets the game." });
}