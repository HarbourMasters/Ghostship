#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include "port/hooks/list/PlayerEvent.h"
#include "port/mods/PortEnhancements.h"

// Entry point for the module, run once on game boot
void Rando::ActorBehavior::Init() {
    REGISTER_LISTENER(SpawnMacroObject, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        SpawnMacroObject* ev = (SpawnMacroObject*)event;
		if (!IS_RANDO) {
			return;
		}
		int16_t model = *(ev->model);
        int16_t posX = ev->posX;
		int16_t posY = ev->posY;
        int16_t posZ = ev->posZ;
        
		//SPDLOG_INFO("ID: {} | posX: {} | posY: {} | posZ: {}", model, posX, posY, posZ);

		Vec3s location;
        location[0] = ev->posX;
        location[1] = ev->posY;
        location[2] = ev->posZ;
        
		RandoCheckId randoCheckId = Rando::StaticData::GetCheckByLocation(ev->posX, ev->posY, ev->posZ);
		if (randoCheckId != RC_UNKNOWN) {
            RandoItemId randoItemId = Rando::StaticData::Checks[randoCheckId].randoItemId;
			if (randoItemId != RI_UNKNOWN) {
                int16_t modelId = Rando::StaticData::GetModelByRandoItem(randoItemId);
                *(ev->model) = modelId;
                *(ev->behavior) = Rando::StaticData::GetBehaviorByModel(modelId);
			}
		}
	});
}

