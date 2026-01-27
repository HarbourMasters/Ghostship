#include "CustomItem.h"
#include "port/Rando/Logic/Logic.h"
#include "port/Rando/CheckTracker/CheckTracker.h"
#include "port/ui/Notification.h"

extern "C" {
#include "game/object_list_processor.h"
#include "include/behavior_data.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
#include "game/interaction.h"
#include "audio/external.h"
#include "game/sound_init.h"
#include "game/mario.h"

// Asset Headers
#include "include/assets/textures/segment2.h"
}

std::map<RandoCheckId, struct Object*> spawnedRandoObjects;
int16_t CustomItem::redCoinsCollected = 0;

std::vector<int32_t> starActParams = {
    0, 16777216, 33554432, 50331648, 67108864, 83886080, 100663296,
};

struct Object* AssignParentObject() {
    for (auto& [randoCheck, spawnedObj] : spawnedRandoObjects) {
        if (Rando::StaticData::Checks[randoCheck].randoCheckType == RCTYPE_STAR_RED_COIN) {
            return spawnedObj;
        }
    }
    return NULL;
}

void CreateCollectNotification(const char* texture, std::string text, ImVec4 textColor) {
    if (texture == texture_hud_char_coin && CustomItem::redCoinsCollected > 1) {
        text += "'s";
    }

    text += " collected!";

    Notification::Emit({ .itemIcon = texture, .message = text, .messageColor = textColor });
}

void CustomItem::ClearSpawnedObjects() {
    spawnedRandoObjects.clear();
}

void CustomItem::ObjectCollected(int16_t type, struct MarioState* mario, struct Object* object) {
    SPDLOG_INFO("Check ID: {} - Collected", std::to_string(object->unused1));

    for (auto& shuffled : Rando::Logic::shuffledPool) {
        if (shuffled.randoCheckId == object->unused1) {
            shuffled.obtained = true;
            RANDO_SAVE_CHECKS(selectedFileNum)[shuffled.randoCheckId].obtained = true;
        }
    }

    switch (type) {
        case TYPE_COIN:
            spawn_object(object, MODEL_SPARKLES, bhvGoldenCoinSparkles);
            if (object->behavior == bhvRedCoin) {
                CustomItem::redCoinsCollected++;
                CreateCollectNotification("Red Coin Icon", std::to_string(CustomItem::redCoinsCollected) + "x Red Coin",
                                          ImVec4(1, 0, 0, 1));

                if (CustomItem::redCoinsCollected != 8) {
                    if (object->parentObj == nullptr) {
                        object->parentObj = object;
                    }
                    object->parentObj->oHiddenStarTriggerCounter = redCoinsCollected;

                    struct Object* spawnNumber;
                    spawnNumber = spawn_object_relative(CustomItem::redCoinsCollected, 0, 0, 0, object, MODEL_NUMBER,
                                                        bhvOrangeNumber);
                    spawnNumber->oPosY += 25.0f;
                } else {
                    object->parentObj->oHiddenStarTriggerCounter = 8;
                }
                play_sound(SOUND_MENU_COLLECT_RED_COIN + ((CustomItem::redCoinsCollected - 1) << 16),
                           gGlobalSoundSource);
            } else {
                CreateCollectNotification("Blue Coin Icon", "Blue Coin", ImVec4(0, 0, 1, 1));
            }
            mario->numCoins += object->oDamageOrCoinValue;
            mario->healCounter += 4 * object->oDamageOrCoinValue;
            break;
        case TYPE_STAR: {
            CreateCollectNotification(texture_hud_char_star, "Course " + std::to_string(object->unused2) + " Star",
                                      ImVec4(1, 1, 0, 1));
            play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
            spawn_object(object, MODEL_NONE, bhvStarKeyCollectionPuffSpawner);
            int16_t starAct = object->unused2;

            save_file_collect_star_or_key(mario->numCoins, starAct);
            mario->numStars = save_file_get_total_star_count(gCurrSaveFileNum - 1, COURSE_MIN - 1, COURSE_MAX - 1);
            save_file_do_save(selectedFileNum);

            break;
        }
        default:
            break;
    }

    if (spawnedRandoObjects.find((RandoCheckId)object->unused1) == spawnedRandoObjects.end()) {
        spawnedRandoObjects.insert({ (RandoCheckId)object->unused1, object });
    }

    spawnedRandoObjects.at((RandoCheckId)object->unused1)->activeFlags = ACTIVE_FLAG_DEACTIVATED;
    spawnedRandoObjects.at((RandoCheckId)object->unused1)->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
}

void CustomItem::SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId, RandoAct randoAct) {
    if (Rando::Logic::IsBlueSwitchActivated(randoCheckId)) {
        object->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
        object->oIntangibleTimer = -1;
        if (modelId == MODEL_STAR) {
            object->oBehParams = starActParams[randoAct];
        }
    } else {
        switch (modelId) {
            case MODEL_BLUE_COIN:
                object->oAction = HIDDEN_BLUE_COIN_ACT_ACTIVE;
                break;
            case MODEL_RED_COIN:
                break;
            case MODEL_STAR:
                object->oBehParams = starActParams[randoAct];
                if (CVarGetInteger("gEnhancements.StarNoExit", 0)) {
                    object->oInteractionSubtype |= INT_SUBTYPE_NO_EXIT;
                }
                break;
            default:
                break;
        }
    }
}

void CustomItem::SpawnObject(u32 modelId, const BehaviorScript* behavior, s16 x, s16 y, s16 z, s32 param,
                             RandoCheckId randoCheckId, RandoAct randoAct) {
    struct Object* object =
        spawn_object_abs_with_rot(&gMacroObjectDefaultParent, 0, modelId, behavior, x, y, z, 0, 0, 0);
    CustomItem::SetBehavior(object, modelId, randoCheckId, randoAct);
    object->unused1 = randoCheckId;
    object->unused2 = randoAct;

    if (param != NULL) {
        object->oBehParams2ndByte = param;
    }

    spawnedRandoObjects.insert({ randoCheckId, object });
}
