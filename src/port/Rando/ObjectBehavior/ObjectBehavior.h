#ifndef RANDO_OBJECT_BEHAVIOR_H
#define RANDO_OBJECT_BEHAVIOR_H

#include "port/Rando/Rando.h"

extern "C" {
#include "game/object_helpers.h"
#include "include/level_commands.h"
}

namespace Rando {

namespace ObjectBehavior {

void Init();
void ModifyBlueCoinSwitchBehavior();
void ModifyRedCoinBehavior();

} // namespace ObjectBehavior

} // namespace Rando

#endif // RANDO_OBJECT_BEHAVIOR_H
