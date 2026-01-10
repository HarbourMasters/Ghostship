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

#include "game/level_update.h"
#include "game/ingame_menu.h"
#include "engine/geo_layout.h"
// #include "game/area.h"
// #include "game/main.h"
extern "C" {
void handle_nmi_request(void);
void thread5_game_loop(void);
void clear_areas(void);
extern s8 gResetTimer;            // main.h
extern s16 gMenuOptSelectIndex;   // area.h
extern struct Area* gCurrentArea; // area.h
extern s16 gMenuMode;             // ingame_menu.c
extern u32 gGlobalTimer;          // game_init.c
}

static bool ResetHandler(std::shared_ptr<Ship::Console> Console, std::vector<std::string> args, std::string* output) {
    // FIXME: Consistently seems to crash at alloc_only_pool_alloc after 4 times of loading into a level and resetting
    handle_nmi_request();
    gResetTimer = 0;
    clear_areas();
    gHudDisplay.flags = HUD_DISPLAY_NONE;
    gMenuMode = MENU_MODE_NONE;
    thread5_game_loop();
    return 0;
}

void DevConsole_Init(void) {
    CMD_REGISTER("reset", { ResetHandler, "Resets the game." });
}