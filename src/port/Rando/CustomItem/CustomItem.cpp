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

void CustomItem::ClearSpawnedObjects() {
    spawnedRandoObjects.clear();
}

void CreateCollectNotification(const char* texture, std::string text, ImVec4 textColor) {
    if (texture == texture_hud_char_coin && CustomItem::redCoinsCollected > 1) {
        text += "'s";
    }

    text += " collected!";

    Notification::Emit({ .itemIcon = texture, .message = text, .messageColor = textColor });
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
                CreateCollectNotification("Red Coin Icon",
                                          std::to_string(CustomItem::redCoinsCollected) + "x Red Coin",
                                          ImVec4(1, 0, 0, 1));

                if (CustomItem::redCoinsCollected != 8) {
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
            }
            mario->numCoins += object->oDamageOrCoinValue;
            mario->healCounter += 4 * object->oDamageOrCoinValue;
            if (COURSE_IS_MAIN_COURSE(gCurrCourseNum) && mario->numCoins - object->oDamageOrCoinValue < 100 &&
                mario->numCoins >= 100) {
                Rando::StaticData::RandoStaticCheck randoStaticCheck =
                    Rando::StaticData::GetShuffledRandoStaticCheck(0, 0, 0);
                if (randoStaticCheck.randoCheckId != RC_UNKNOWN) {
                    int16_t modelId = Rando::StaticData::GetModelByRandoItem(randoStaticCheck.randoItemId);
                    const BehaviorScript* behavior = modelId == MODEL_BLUE_COIN ? bhvHiddenBlueCoin
                                                     : modelId == MODEL_STAR
                                                         ? bhvSpawnedStarNoLevelExit
                                                         : Rando::StaticData::GetBehaviorByModel(modelId);

                    CustomItem::SpawnObject(modelId, behavior, object->rawData.asF32[0x6],
                                            object->rawData.asF32[0x7] + 250, object->rawData.asF32[0x8], NULL,
                                            randoStaticCheck.randoCheckId, randoStaticCheck.actData);
                }
            }
            break;
        case TYPE_STAR: {
            CreateCollectNotification(texture_hud_char_star, "Course " + std::to_string(object->unused2) + " Star",
                                      ImVec4(1, 1, 0, 1));
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

    object->activeFlags = ACTIVE_FLAG_DEACTIVATED;
}

void CustomItem::SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId, RandoAct randoAct) {
    if (Rando::Logic::IsBlueSwitchActivated(randoCheckId)) {
        object->header.gfx.node.flags &= ~GRAPH_RENDER_ACTIVE;
        object->oIntangibleTimer = -1;
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
