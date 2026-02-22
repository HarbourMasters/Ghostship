#include "Achievements.h"

#include <unordered_map>

#include "behavior_data.h"
#include "sm64.h"
#include "course_table.h"
#include "port/ui/cvar_prefixes.h"
#include "game/save_file.h"
#include "buffers/buffers.h"
#include "port/ShipInit.hpp"
#include "port/Rando/Types.h"
#include "port/hooks/Events.h"
#include "game/level_update.h"
#include "port/util/GraphNode.h"
#include "port/ui/Notification.h"
#include "game/object_list_processor.h"

static size_t order = 0;
static int16_t selectedFile = 0;
static BossBattleType bossBattleType = BOSS_BATTLE_NONE;

std::unordered_map<std::string, AchievementProgress> gAchievementProgress;

#define R(id, cat, name, description, icon, ...)     \
    {                                                \
        id, {                                        \
            cat, name, icon, description, order++, { \
                __VA_ARGS__                          \
            }                                        \
        }                                            \
    }

#define P(id, cat, name, description, icon, maxProgress, ...)                   \
    {                                                                           \
        id, {                                                                   \
            cat, name, icon, description, order++, { __VA_ARGS__ }, maxProgress \
        }                                                                       \
    }

std::unordered_map<std::string, Achievement> gAchievementList = {
    P("Get1Star", AchievementCategory::Stars, "Your Journey Begins", "Get a Star", "stars.1", 1),
    P("Talk25Times", AchievementCategory::Extras, "Olympic Talker", "Talk to NPCs 25 Times", "extras.talker", 25),
    P("Jump1000Times", AchievementCategory::Extras, "Olympic Jumper", "Jump 1000 Times", "extras.jumper", 1000),
    P("Slide20Times", AchievementCategory::Extras, "Burned Ass", "Go Down The Slide 20 Times", "extras.carpet-burn",
      20),
    R("DeathByEnemy", AchievementCategory::Deaths, "Watch Your Step", "Die by an Enemy", "deaths.standard"),
    R("DeathByFalling", AchievementCategory::Deaths, "Gravity Is A Myth", "Die by Falling", "deaths.falling"),
    R("DeathByCrushing", AchievementCategory::Deaths, "Space Jam", "Die by Being Crushed", "deaths.crushed"),
    R("DeathByDrowning", AchievementCategory::Deaths, "Under The Sea", "Die by Drowning", "deaths.drowning"),
    R("DeathByFire", AchievementCategory::Deaths, "Roasted Mario", "Die by Fire or Lava", "deaths.fire"),
    R("DeathByQuickSand", AchievementCategory::Deaths, "Sink Or Swim", "Die in Quick Sand", "deaths.quicksand"),
    R("DeathByBoss", AchievementCategory::Deaths, "Git Gud", "Get Defeated by a Boss", "deaths.boss"),
    R("DeathByBowser", AchievementCategory::Deaths, "Bad Ending", "Get Defeated by Bowser", "deaths.bowser"),
    R("DefeatKingBobomb", AchievementCategory::Bosses, "Explosive Test", "Defeat King Bob-omb", "bosses.big-bob"),
    R("ReleaseChainChomp", AchievementCategory::Extras, "Chomp-Chomp!", "Release the Chain Chomp",
      "extras.chain-chomp"),
    R("DefeatKingWhomp", AchievementCategory::Bosses, "Come On And Slam", "Defeat King Whomp", "bosses.king-whomp"),
    R("Get6MainStars", AchievementCategory::Levels, "F Rank", "Get all 6 Main Stars in One Level", "ranks.f"),
    R("Get100CoinStar", AchievementCategory::Levels, "E Rank", "Get a 100-Coin Star", "ranks.e", "Get6MainStars"),
    P("Get8Stars", AchievementCategory::Stars, "You feel a strong power", "Get 8 Stars", "stars.8", 8, "Get1Star"),
    R("DefeatBowser1", AchievementCategory::Bosses, "Bowser Trapped In The Dark", "Defeat Bowser in the Dark World",
      "bosses.bowser-1"),
    R("UnlockWingCap", AchievementCategory::Caps, "Super Man-rio", "Unlock the Wing Cap", "cap.wing"),
    R("DefeatAllBooses", AchievementCategory::Bosses, "Boo Who?", "Defeat All Boos in the Haunted House",
      "bosses.big-boo"),
    R("DefeatMrI", AchievementCategory::Bosses, "I vs Eye", "Defeat Mr. I", "bosses.mr-i"),
    R("UnlockMetalCap", AchievementCategory::Caps, "Heavy-Headed", "Unlock the Metal Cap", "cap.metal"),
    R("UnlockVanishCap", AchievementCategory::Caps, "Wait, Where Is He?", "Unlock the Vanish Cap", "cap.vanish"),
    R("DefeatAllBigBullies", AchievementCategory::Bosses, "The Real Bully", "Defeat All Big Bullies From LLL",
      "bosses.big-bully"),
    R("DefeatEyerok", AchievementCategory::Bosses, "Welcome To The Jam", "Defeat Eyerok", "bosses.eyerok"),
    P("Get30Stars", AchievementCategory::Stars, "Earning a Quarter", "Get 30 Stars", "stars.30", 30, "Get8Stars"),
    P("Get31Stars", AchievementCategory::Stars, "Extra Cent", "Get 31 Stars", "stars.31", 31, "Get30Stars"),
    R("DefeatBowser2", AchievementCategory::Bosses, "Bowser Burned By The Lava", "Defeat Bowser in the Fire Sea",
      "bosses.bowser-2"),
    P("Get50Stars", AchievementCategory::Stars, "Lucky Eight", "Get 50 Stars", "stars.50", 50, "Get31Stars"),
    R("DefeatWiggler", AchievementCategory::Bosses, "Insecticide", "Defeat Wiggler", "bosses.wiggler"),
    P("Get70Stars", AchievementCategory::Stars, "Halfway There", "Get 70 Stars", "stars.70", 70, "Get50Stars"),
    R("DefeatBowser3", AchievementCategory::Bosses, "Final Showdown", "Defeat Bowser in the Sky", "bosses.bowser-3"),
    R("WatchEnding", AchievementCategory::Extras, "The Cake Is A Lie?!", "Watch the game ending", "extras.cake"),
    R("BeatEveryRace", AchievementCategory::Extras, "Olympic Runner", "Beat Every Racing Challenge", "extras.runner"),
    R("GrabSwimmingStars", AchievementCategory::Extras, "Olympic Swimmer",
      "Grab every star that needs Metal Cap without it", "extras.swimmer"),
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
    P("Get120Stars", AchievementCategory::Stars, "The Completionist", "Get 120 Stars", "stars.120", 120, "Get70Stars"),
    R("TalkWithYoshi", AchievementCategory::Extras, "It Is You?", "Talk with Yoshi on the Roof", "extras.yoshi"),
    R("DefeatBowser3WithAllStars", AchievementCategory::Bosses, "True Ending",
      "Defeat Bowser in the Sky with 120 Stars", "bosses.bowser-3-with-120-stars", "The Completionist"),
};

int Achievement_GetObjectCount(std::vector<int32_t> models) {
    int count = 0;
    for (int i = 0; i < NUM_OBJ_LISTS; i++) {
        ObjectNode* listHead = &gObjectLists[i];

        const Object* next = reinterpret_cast<Object*>(listHead->next);
        while (next != reinterpret_cast<Object*>(listHead)) {
            GraphNodeID model = GraphNodeManager::GetNodeID(next->header.gfx.sharedChild);
            if (std::find(models.begin(), models.end(), model) != models.end()) {
                count++;
            }
            next = reinterpret_cast<Object*>(next->header.next);
        }
    }
    return count;
}

Achievement* Achievement_FindByID(const std::string& id) {
    if (!gAchievementList.contains(id)) {
        SPDLOG_ERROR("Trying to find non-existent achievement with id {}", id);
        return nullptr;
    }

    return &gAchievementList[id];
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
            } else {
                SPDLOG_INFO("Progressed achievement {}: {}/{}", achievement->name, progress, achievement->maxProgress);
            }

            // Save after each achievement progress update to prevent loss of progress on crash
        } else {
            SPDLOG_WARN("Trying to progress achievement {} that is already achieved", achievement->name);
        }
    } else {
        SPDLOG_ERROR("Trying to progress non-existent achievement with id {}", id);
    }
}

void Achievement_ProgressByCategory(AchievementCategory category, int32_t amount) {
    for (auto& [id, achievement] : gAchievementList) {
        if (static_cast<int32_t>(achievement.category) & static_cast<int32_t>(category)) {
            auto& [progress, achieved] = gAchievementProgress[id];

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

void Achievement_LoadTexture(const std::string& id) {
    const Achievement& achievement = gAchievementList[id];

    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(RESOURCE_TYPE_GUI_TEXTURE);
    initData->ResourceVersion = 0;
    initData->Path = "textures/achievements/" + std::string(achievement.icon) + ".png";
    auto texture = std::static_pointer_cast<Ship::GuiTexture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(initData->Path, false, initData));

    Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromResource(achievement.icon, texture);

    for (int32_t i = 0; i < texture->Metadata.Width * texture->Metadata.Height * 4; i += 4) {
        const uint8_t r = texture->Data[i];
        const uint8_t g = texture->Data[i + 1];
        const uint8_t b = texture->Data[i + 2];

        const uint8_t gray = static_cast<uint8_t>(0.21f * r + 0.72f * g + 0.07f * b);

        texture->Data[i] = gray;
        texture->Data[i + 1] = gray;
        texture->Data[i + 2] = gray;
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromResource(
        std::string(achievement.icon) + ".locked", texture);
}

void Achievements_Load(IEvent* event) {
    const OnGameFileLoad* ev = reinterpret_cast<OnGameFileLoad*>(event);
    selectedFile = ev->fileNum - 1;

    AchievementSaveData* saveData = &gSaveBuffer.files[selectedFile]->shipSaveData.achievementSaveData;

    if (!CVarGetInteger(CVAR_ENHANCEMENT("Achievements"), 0) && !HAS_ACHIEVEMENTS(selectedFile)) {
        memset(saveData, 0, sizeof(AchievementSaveData));
        return;
    }

    if (!HAS_ACHIEVEMENTS(selectedFile)) {
        size_t idx = 0;
        for (const auto& [id, achievement] : gAchievementList) {
            saveData->entries[idx++].id = id.c_str();
        }

        gSaveBuffer.files[selectedFile]->shipSaveData.features.achievements = true;
        save_file_do_save(selectedFile);
    } else {
        for (size_t i = 0; i < gAchievementList.size(); i++) {
            auto& [id, progress] = saveData->entries[i];

            gAchievementProgress[id].progress = progress;
            gAchievementProgress[id].achieved = progress >= gAchievementList[id].maxProgress;
        }
    }
}

void Achievements_Save(IEvent* event) {
    if (!HAS_ACHIEVEMENTS(selectedFile)) {
        return;
    }

    AchievementSaveData* saveData = &gSaveBuffer.files[selectedFile]->shipSaveData.achievementSaveData;

    saveData->cheated = false; // TODO: Implement cheat detection

    size_t index = 0;
    for (const auto& [id, progress] : gAchievementProgress) {
        saveData->entries[index].id = id.c_str();
        saveData->entries[index].progress = progress.progress;
        index++;
    }
}

void Achievements_Init() {
    for (auto& [id, achievement] : gAchievementList) {
        gAchievementProgress[id] = { 0, false };
        Achievement_LoadTexture(id);
    }

    // Register event listeners
    REGISTER_LISTENER(OnGameFileLoad, EVENT_PRIORITY_NORMAL, Achievements_Load);
    REGISTER_LISTENER(OnGameFileSave, EVENT_PRIORITY_NORMAL, Achievements_Save);

    REGISTER_LISTENER(ItemCollected, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

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
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        const CapSwitchActivated* ev = reinterpret_cast<CapSwitchActivated*>(event);

        SPDLOG_INFO("Cap Switch Activated: {}", static_cast<int>(ev->type));

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
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

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
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        const Object* interactObj = gMarioState->interactObj;

        if (interactObj != nullptr) {
            if (interactObj->behavior == segmented_to_virtual(bhvYoshi)) {
                Achievement_Progress("TalkWithYoshi");
            }
        }

        if (gCurrLevelNum == LEVEL_LLL && gCurrAreaIndex != 2) {
            const int count = Achievement_GetObjectCount({ MODEL_BULLY, MODEL_BULLY_BOSS });
            if (count == 0) {
                Achievement_Progress("DefeatAllBigBullies");
            }
        }

        if (gCurrLevelNum == LEVEL_BBH) {
            const int count = Achievement_GetObjectCount({ MODEL_BOO });
            if (count == 0) {
                Achievement_Progress("DefeatAllBooses");
            }
        }
    });

    REGISTER_LISTENER(BossBattleStarted, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        bossBattleType = reinterpret_cast<BossBattleStarted*>(event)->type;
        SPDLOG_INFO("Boss battle started: {}", static_cast<int>(bossBattleType));
    });

    REGISTER_LISTENER(BossBattleEnded, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        bossBattleType = BOSS_BATTLE_NONE;
        SPDLOG_INFO("Boss battle ended");
    });

    REGISTER_LISTENER(PlayerDeath, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const PlayerDeath* ev = reinterpret_cast<PlayerDeath*>(event);

        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        switch (bossBattleType) {
            case BOSS_BATTLE_KOOPA:
            case BOSS_BATTLE_KOOPA_FINAL:
                Achievement_Progress("DeathByBowser");
                break;
            case BOSS_BATTLE_GENERIC:
                Achievement_Progress("DeathByBoss");
                break;
            default:
                break;
        }

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
                Achievement_Progress("DeathByEnemy");
                break;
            }
        }
    });

    REGISTER_LISTENER(PlayerExecuteAction, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        const PlayerExecuteAction* ev = reinterpret_cast<PlayerExecuteAction*>(event);

        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        SPDLOG_INFO("ExecuteAction {:X}", ev->action);

        switch (ev->action) {
            case ACT_BEGIN_SLIDING:
                Achievement_Progress("Slide20Times");
                break;
            case ACT_JUMP:
            case ACT_BACKFLIP:
            case ACT_JUMP_KICK:
            case ACT_DOUBLE_JUMP:
            case ACT_TRIPLE_JUMP:
            case ACT_LONG_JUMP:
                Achievement_Progress("Jump1000Times");
                break;
            case ACT_READING_NPC_DIALOG:
                Achievement_Progress("Talk25Times");
                break;
            default:
                break;
        }
    });

    REGISTER_LISTENER(ChainChompRelease, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        if (!HAS_ACHIEVEMENTS(selectedFile)) {
            return;
        }

        Achievement_Progress("ReleaseChainChomp");
    });
}

static RegisterShipInitFunc initFunc(Achievements_Init);