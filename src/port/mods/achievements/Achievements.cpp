#include "Achievements.h"

#include <unordered_map>

#include "behavior_data.h"
#include "sm64.h"
#include "seq_ids.h"
#include "course_table.h"
#include "game/save_file.h"
#include "audio/external.h"
#include "port/ShipInit.hpp"
#include "port/Rando/Types.h"
#include "port/hooks/Events.h"
#include "game/level_update.h"
#include "port/util/GraphNode.h"
#include "port/ui/Notification.h"
#include "game/object_list_processor.h"

std::unordered_map<std::string, AchievementProgress> gAchievementProgress;

#define R(id, cat, name, description, icon, ...) \
    {                                            \
        #id, cat, name, icon, description, {     \
            __VA_ARGS__                          \
        }                                        \
    }

#define P(id, cat, name, description, icon, maxProgress, ...) \
    { #id, cat, name, icon, description, { __VA_ARGS__ }, maxProgress }

std::vector<Achievement> gAchievementList = {
    // Star Achievements
    P("Get1Star", AchievementCategory::Stars, "Your Journey Begins", "Get a Star", "stars.1", 1),
    P("Get8Stars", AchievementCategory::Stars, "You feel a strong power", "Get 8 Stars", "stars.8", 8, "Get1Star"),
    P("Get30Stars", AchievementCategory::Stars, "Earning a Quarter", "Get 30 Stars", "stars.30", 30, "Get8Stars"),
    P("Get31Stars", AchievementCategory::Stars, "Extra Cent", "Get 31 Stars", "stars.31", 31, "Get30Stars"),
    P("Get50Stars", AchievementCategory::Stars, "Lucky Eight", "Get 50 Stars", "stars.50", 50, "Get31Stars"),
    P("Get70Stars", AchievementCategory::Stars, "Halfway There", "Get 70 Stars", "stars.70", 70, "Get50Stars"),
    P("Get120Stars", AchievementCategory::Stars, "The Completionist", "Get 120 Stars", "stars.120", 120, "Get70Stars"),

    // Cap Achievements
    R("UnlockWingCap", AchievementCategory::Caps, "Super Man-rio", "Unlock the Wing Cap", "cap.wing"),
    R("UnlockMetalCap", AchievementCategory::Caps, "Heavy-Headed", "Unlock the Metal Cap", "cap.metal"),
    R("UnlockVanishCap", AchievementCategory::Caps, "Wait, Where Is He?", "Unlock the Vanish Cap", "cap.vanish"),

    // Level Achievements
    R("Get6MainStars", AchievementCategory::Levels, "F Rank", "Get all 6 Main Stars in One Level", "ranks.f"),
    R("Get100CoinStar", AchievementCategory::Levels, "E Rank", "Get a 100-Coin Star", "ranks.e", "Get6MainStars"),
    R("GetAll100CoinStars", AchievementCategory::Levels, "D Rank", "Get all Coins in One Level", "ranks.d",
      "Get100CoinStar"),
    R("GetAllStarsInBasement", AchievementCategory::Levels, "C Rank", "Get all Main Stars in the Basement", "ranks.c",
      "GetAll100CoinStars"),
    R("GetAllStarsInFloor1", AchievementCategory::Levels, "B Rank", "Get all Main Stars on Floor 1", "ranks.b",
      "GetAllStarsInBasement"),
    R("GetAllStarsInFloor2", AchievementCategory::Levels, "A Rank", "Get all Main Stars on Floor 2", "ranks.a",
      "GetAllStarsInFloor1"),
    R("GetAllStarsInGame", AchievementCategory::Levels, "S Rank", "Get all Main Stars on Floor 3", "ranks.s",
      "GetAllStarsInFloor2"),
    R("GetAllCastleStars", AchievementCategory::Levels, "S+ Rank", "Get all Castle Main Stars", "ranks.splus",
      "GetAllStarsInGame"),

    // Boss Achievements
    R("DefeatKingBobomb", AchievementCategory::Bosses, "Explosive Test", "Defeat King Bob-omb", "bosses.big-bob"),
    R("DefeatMrI", AchievementCategory::Bosses, "I vs Eye", "Defeat Mr. I", "bosses.mr-i"),
    R("DefeatWiggler", AchievementCategory::Bosses, "Insecticide", "Defeat Wiggler", "bosses.wiggler"),
    R("DefeatAllBooses", AchievementCategory::Bosses, "Boo Who?", "Defeat All Boos in the Haunted House",
      "bosses.big-boo"),
    R("DefeatAllBigBullies", AchievementCategory::Bosses, "The Real Bully", "Defeat All Big Bullies From LLL",
      "bosses.big-bully"),
    R("DefeatEyerok", AchievementCategory::Bosses, "Welcome To The Jam", "Defeat Eyerok", "bosses.eyerok"),
    R("DefeatKingWhomp", AchievementCategory::Bosses, "Come On And Slam", "Defeat King Whomp", "bosses.king-whomp"),
    R("DefeatBowser1", AchievementCategory::Bosses, "Bowser Trapped In The Dark", "Defeat Bowser in the Dark World",
      "bosses.bowser-1"),
    R("DefeatBowser2", AchievementCategory::Bosses, "Bowser Burned By The Lava", "Defeat Bowser in the Fire Sea",
      "bosses.bowser-2"),
    R("DefeatBowser3", AchievementCategory::Bosses, "Final Showdown", "Defeat Bowser in the Sky", "bosses.bowser-3"),
    R("DefeatBowser3WithAllStars", AchievementCategory::Bosses, "True Ending",
      "Defeat Bowser in the Sky with 120 Stars", "bosses.bowser-3-with-120-stars", "The Completionist"),

    // Death Achievements
    R("DeathByBoss", AchievementCategory::Deaths, "Git Gud", "Get Defeated by a Boss", "deaths.boss"),
    R("DeathByFalling", AchievementCategory::Deaths, "Gravity Is A Myth", "Die by Falling", "deaths.falling"),
    R("DeathByQuickSand", AchievementCategory::Deaths, "Sink Or Swim", "Die in Quick Sand", "deaths.quicksand"),
    R("DeathByCrushing", AchievementCategory::Deaths, "Space Jam", "Die by Being Crushed", "deaths.crushed"),
    R("DeathByBowser", AchievementCategory::Deaths, "Bad Ending", "Get Defeated by Bowser", "deaths.bowser"),
    R("DeathByEnemy", AchievementCategory::Deaths, "Watch Your Step", "Die by an Enemy", "deaths.standard"),
    R("DeathByDrowning", AchievementCategory::Deaths, "Under The Sea", "Die by Drowning", "deaths.drowning"),
    R("DeathByFire", AchievementCategory::Deaths, "Roasted Mario", "Die by Fire or Lava", "extras.carpet-burn"),

    // Extra Achievements
    R("TalkWithYoshi", AchievementCategory::Extras, "It Is You?", "Talk with Yoshi on the Roof", "extras.yoshi"),
    P("Slide20Times", AchievementCategory::Extras, "Burned Ass", "Go Down The Slide 20 Times", "extras.carpet-burn",
      20),
    R("BeatEveryRace", AchievementCategory::Extras, "Olympic Runner", "Beat Every Racing Challenge", "extras.runner"),
    P("Talk25Times", AchievementCategory::Extras, "Olympic Talker", "Talk to NPCs 25 Times", "extras.talker", 25),
    P("Jump1000Times", AchievementCategory::Extras, "Olympic Jumper", "Jump 1000 Times", "extras.jumper", 1000),
    R("GrabSwimmingStars", AchievementCategory::Extras, "Olympic Swimmer",
      "Grab every star that needs Metal Cap without it", "extras.swimmer"),
    R("WatchEnding", AchievementCategory::Extras, "The Cake Is A Lie?!", "Watch the game ending", "extras.cake"),
    R("ReleaseChainChomp", AchievementCategory::Extras, "Chomp-Chomp!", "Release the Chain Chomp",
      "extras.chain-chomp"),
};

int Achievement_GetObjectCount(std::vector<int32_t> models) {
    int count = 0;
    for (int i = 0; i < NUM_OBJ_LISTS; i++) {
        struct ObjectNode* listHead = &gObjectLists[i];
        struct Object* next = (struct Object*)listHead->next;
        while (next != (struct Object*)listHead) {
            GraphNodeID model = GraphNodeManager::GetNodeID(next->header.gfx.sharedChild);
            if (std::find(models.begin(), models.end(), model) != models.end()) {
                count++;
            }
            next = (struct Object*)next->header.next;
        }
    }
    return count;
}

Achievement* Achievement_FindByID(const std::string& id) {
    for (auto& achievement : gAchievementList) {
        if (achievement.id == id) {
            return &achievement;
        }
    }
    return nullptr;
}

void Achievement_Progress(const std::string& id, const int32_t amount = 1) {
    const Achievement* achievement = Achievement_FindByID(id);
    if (achievement) {
        auto& [progress, achieved] = gAchievementProgress[id];

        if (!achieved) {
            progress += amount;
            if (progress >= achievement->maxProgress) {
                achieved = true;
                Notification::EmitAchievement(achievement->icon, achievement->name, 0);
            }

            // Save after each achievement progress update to prevent loss of progress on crash
        }
    }
}

void Achievement_ProgressByCategory(AchievementCategory category, int32_t amount) {
    for (auto& achievement : gAchievementList) {
        if (static_cast<int32_t>(achievement.category) & static_cast<int32_t>(category)) {
            auto& [progress, achieved] = gAchievementProgress[achievement.id];

            if (!achieved) {
                progress += amount;
                if (progress >= achievement.maxProgress) {
                    achieved = true;
                    Notification::EmitAchievement(achievement.icon, achievement.name, 0);
                }

                // Save after each achievement progress update to prevent loss of progress on crash
            }
        }
    }
}

AchievementProgress* Achievement_GetProgress(const std::string& id) {
    return &gAchievementProgress[id];
}

void Achievements_Init() {
    for (auto& achievement : gAchievementList) {
        // Set default progress to 0 and not achieved for each achievement
        gAchievementProgress[achievement.id] = { 0, false };

        // Load achievement icons as ImGui textures
        Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromRawImage(
            achievement.icon, "textures/achievements/" + std::string(achievement.icon) + ".png");
    }

    // Register event listeners
    REGISTER_LISTENER(ItemCollected, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const ItemCollected* ev = reinterpret_cast<ItemCollected*>(event);
        if (ev->type == TYPE_STAR) {
            const int16_t slot = gCurrSaveFileNum - 1;
            const uint32_t starFlags = save_file_get_star_flags(slot, gCurrCourseNum - 1);

            Achievement_ProgressByCategory(AchievementCategory::Stars, 1);
            if ((starFlags & 0x3F) == 0x3F) {
                Achievement_Progress("Get6MainStars");
            }

            if (save_file_get_total_star_count(slot, COURSE_BBH, COURSE_LLL) >= 21) {
                Achievement_Progress("GetAllStarsInBasement");
            }

            if (save_file_get_total_star_count(slot, COURSE_MIN, COURSE_MAX) >= 120) {
                Achievement_Progress("Get120Stars");
            }
        }

        if (ev->type == TYPE_COIN && gMarioState->numCoins >= 100) {
            Achievement_Progress("Get100CoinStar");
        }
    });

    REGISTER_LISTENER(CapSwitchActivated, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const CapSwitchActivated* ev = reinterpret_cast<CapSwitchActivated*>(event);
        switch (ev->type) {
            case CAP_SWITCH_WING:
                Achievement_Progress("UnlockWingCap");
                break;
            case CAP_SWITCH_METAL:
                Achievement_Progress("UnlockMetalCap");
                break;
            case CAP_SWITCH_VANISH:
                Achievement_Progress("UnlockVanishCap");
                break;
        }
    });

    REGISTER_LISTENER(BossDefeated, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const BossDefeated* ev = reinterpret_cast<BossDefeated*>(event);
        switch (ev->type) {
            case BOSS_TYPE_KING_BOBOMB:
                Achievement_Progress("DefeatKingBobomb");
                break;
            case BOSS_TYPE_MR_I:
                Achievement_Progress("DefeatMrI");
                break;
            case BOSS_TYPE_WIGGLER:
                Achievement_Progress("DefeatWiggler");
                break;
            case BOSS_TYPE_EYEROK:
                Achievement_Progress("DefeatEyerok");
                break;
            case BOSS_TYPE_KING_WHOMP:
                Achievement_Progress("DefeatKingWhomp");
                break;
            case BOSS_TYPE_BOWSER_BITDW:
                Achievement_Progress("DefeatBowser1");
                break;
            case BOSS_TYPE_BOWSER_BITFS:
                Achievement_Progress("DefeatBowser2");
                break;
            case BOSS_TYPE_BOWSER_BITS:
                if (save_file_get_total_star_count(gCurrSaveFileNum - 1, COURSE_MIN, COURSE_MAX) >= 120) {
                    Achievement_Progress("DefeatBowser3WithAllStars");
                } else {
                    Achievement_Progress("DefeatBowser3");
                }
            default:
                break;
        }
    });

    REGISTER_LISTENER(GameFrameUpdate, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const Object* interactObj = gMarioState->interactObj;

        if (interactObj != nullptr) {
            if (interactObj->behavior == segmented_to_virtual(bhvYoshi)) {
                Achievement_Progress("TalkWithYoshi");
            }
        }

        if (gCurrLevelNum == LEVEL_LLL && gCurrAreaIndex != 2) {
            int count = Achievement_GetObjectCount({ MODEL_BULLY, MODEL_BULLY_BOSS });
            if (count == 0) {
                Achievement_Progress("DefeatAllBigBullies");
            }
        }

        if (gCurrLevelNum == LEVEL_BBH) {
            int count = Achievement_GetObjectCount({ MODEL_BOO });
            if (count == 0) {
                Achievement_Progress("DefeatAllBooses");
            }
        }
    });

    REGISTER_LISTENER(PlayerDeath, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const PlayerDeath* ev = reinterpret_cast<PlayerDeath*>(event);
        switch (ev->type) {
            case DEATH_TYPE_FALL:
                Achievement_Progress("DeathByFalling");
                break;
            case DEATH_TYPE_QUICKSAND:
                Achievement_Progress("DeathByQuickSand");
                break;
            case DEATH_TYPE_SQUISHED:
                Achievement_Progress("DeathByCrushing");
                break;
            case DEATH_TYPE_DROWNING:
                Achievement_Progress("DeathByDrowning");
                break;
            case DEATH_TYPE_LAVA:
            case DEATH_TYPE_FIRE:
                Achievement_Progress("DeathByFire");
                break;
            default: {
                if (is_sequence_playing(SEQ_EVENT_BOSS)) {
                    Achievement_Progress("DeathByBoss");
                } else if (is_sequence_playing(SEQ_LEVEL_BOSS_KOOPA) ||
                           is_sequence_playing(SEQ_LEVEL_BOSS_KOOPA_FINAL)) {
                    Achievement_Progress("DeathByBowser");
                } else {
                    Achievement_Progress("DeathByEnemy");
                }
                break;
            }
        }
    });

    REGISTER_LISTENER(PlayerExecuteAction, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const PlayerExecuteAction* ev = reinterpret_cast<PlayerExecuteAction*>(event);
        switch (ev->action) {
            case ACT_BUTT_SLIDE:
                Achievement_Progress("Slide20Times");
                break;
            case ACT_JUMP:
            case ACT_JUMP_KICK:
            case ACT_DOUBLE_JUMP:
            case ACT_TRIPLE_JUMP:
            case ACT_LONG_JUMP:
            case ACT_JUMP_LAND:
                Achievement_Progress("Jump1000Times");
                break;
            case ACT_READING_NPC_DIALOG:
                Achievement_Progress("Talk25Times");
                break;
            default:
                break;
        }
    });

    REGISTER_LISTENER(ChainChompRelease, EVENT_PRIORITY_NORMAL,
                      [](IEvent* event) { Achievement_Progress("ReleaseChainChomp"); });
}

static RegisterShipInitFunc initFunc(Achievements_Init);