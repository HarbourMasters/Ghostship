#ifndef RANDO_LOGIC_H
#define RANDO_LOGIC_H

#include "port/Rando/Rando.h"
#include "port/ShipUtils.h"

namespace Rando {

namespace Logic {

extern std::map<RandoCheckId, RandoItemId> shuffledList;
extern std::vector<RandoCheckId> shuffledChecks;
extern std::vector<RandoItemId> shuffledItems;

void shuffleRandoItems(std::vector<RandoItemId>& shuffledItems);
void GenerateShuffleList();

// Logic Operators

} // namespace Logic

} // namespace Rando

#endif // RANDO_LOGIC_H