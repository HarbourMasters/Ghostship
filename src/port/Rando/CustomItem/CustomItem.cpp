#include "CustomItem.h"

extern "C" {
#include "game/object_list_processor.h"
#include "include/behavior_data.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
#include "game/interaction.h"
#include "audio/external.h"
#include "game/sound_init.h"
#include "game/mario.h"
}

std::map<RandoCheckId, struct Object*> spawnedRandoObjects;
int16_t CustomItem::redCoinsCollected = 7;

std::vector<int32_t> starActParams = {
    0, 16777216, 33554432, 50331648, 67108864, 83886080, 100663296,
};

void CustomItem::ObjectCollected(int16_t type, struct MarioState* mario, struct Object* object) {
    // TODO: Implement Check Tracker functionality using this.
    SPDLOG_INFO("Check ID: {} - Collected", std::to_string(object->unused1));
    switch (type) {
        case TYPE_COIN:
            spawn_object(object, MODEL_SPARKLES, bhvGoldenCoinSparkles);
            if (object->behavior == bhvRedCoin) {
                CustomItem::redCoinsCollected++;
                if (CustomItem::redCoinsCollected != 8) {
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
                // TODO: Replace with CustomItem::SpawnObject for 100 Coin Star
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
            spawn_object(object, MODEL_NONE, bhvStarKeyCollectionPuffSpawner);
            bool shouldExitLevel = (object->oInteractionSubtype & INT_SUBTYPE_NO_EXIT) == 0 ||
                                   !CVarGetInteger("gEnhancements.StarNoExit", 0);
            bool isGrandStar = (object->oInteractionSubtype & INT_SUBTYPE_GRAND_STAR) != 0;
            u32 starGrabAction = ACT_STAR_DANCE_EXIT;
            int16_t starAct = object->unused2;

            if (shouldExitLevel) {
                mario->hurtCounter = 0;
                mario->healCounter = 0;
                if (mario->capTimer > 1) {
                    mario->capTimer = 1;
                }
            }

            if (!shouldExitLevel) {
                starGrabAction = ACT_STAR_DANCE_NO_EXIT;
            }

            if (mario->action & ACT_FLAG_SWIMMING) {
                starGrabAction = ACT_STAR_DANCE_WATER;
            }

            if (mario->action & ACT_FLAG_METAL_WATER) {
                starGrabAction = ACT_STAR_DANCE_WATER;
            }

            if (mario->action & ACT_FLAG_AIR) {
                starGrabAction = ACT_FALL_AFTER_STAR_GRAB;
            }

            // TODO: Save to file once JSON Saves are implemented.
            save_file_collect_star_or_key(mario->numCoins, starAct);
            mario->numStars = save_file_get_total_star_count(gCurrSaveFileNum - 1, COURSE_MIN - 1, COURSE_MAX - 1);

            if (shouldExitLevel) {
                drop_queued_background_music();
                fadeout_level_music(126);
            }

            play_sound(SOUND_MENU_STAR_SOUND, mario->marioObj->header.gfx.cameraToObject);
            if (!ROM_JP) {
                update_mario_sound_and_camera(mario);
            }

            if (isGrandStar) {
                set_mario_action(mario, ACT_JUMBO_STAR_CUTSCENE, 0);
            } else {
                set_mario_action(mario, starGrabAction, !shouldExitLevel + 2 * isGrandStar);
            }
            break;
        }
        default:
            break;
    }

    object->activeFlags = ACTIVE_FLAG_DEACTIVATED;
}

void CustomItem::SetBehavior(struct Object* object, u32 modelId, RandoCheckId randoCheckId, RandoAct randoAct) {
    // TODO: Change this to use the Rando::StaticData::Checks and look for RI_COIN_BLUE
    if (Rando::StaticData::Checks[randoCheckId].randoItemId == RI_COIN_BLUE) {
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
