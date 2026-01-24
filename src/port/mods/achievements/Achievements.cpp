#include "Achievements.h"

#include <unordered_map>

#define R(id, name, description, icon, ...) { #id, name, icon, description, { __VA_ARGS__ } }
#define P(id, name, description, icon, maxProgress, ...) { #id, name, icon, description, { __VA_ARGS__ }, maxProgress }

std::vector<Achievement> gAchievementList = {
    // Star Achievements
    P("Get1Star", "Your Journey Begins", "Get a Star", "stars.1", 1),
    P("Get8Stars", "You feel a strong power", "Get 8 Stars", "stars.8", 8, "Get1Star"),
    P("Get30Stars", "Earning a Quarter", "Get 30 Stars", "stars.30", 30, "Get8Stars"),
    P("Get31Stars", "Extra Cent", "Get 31 Stars", "stars.31", 31, "Get30Stars"),
    P("Get50Stars", "Lucky Eight", "Get 50 Stars", "stars.50", 50, "Get31Stars"),
    P("Get70Stars", "Halfway There", "Get 70 Stars", "stars.70", 70, "Get50Stars"),
    P("Get120Stars", "The Completionist", "Get 120 Stars", "stars.120", 120, "Get70Stars"),

    // Cap Achievements
    R("UnlockWingCap", "Super Man-rio", "Unlock the Wing Cap", "cap.wing"),
    R("UnlockMetalCap", "Heavy-Headed", "Unlock the Metal Cap", "cap.metal"),
    R("UnlockVanishCap", "Wait, Where Is He?", "Unlock the Vanish Cap", "cap.vanish"),

    // Level Achievements
    R("Get6MainStars", "F Rank", "Get all 6 Main Stars in One Level", "ranks.f"),
    R("Get100CoinStar", "E Rank", "Get a 100-Coin Star", "ranks.e", "Get6MainStars"),
    R("GetAll100CoinStars", "D Rank", "Get all Coins in One Level", "ranks.d", "Get100CoinStar"),
    R("GetAllStarsInBasement", "C Rank", "Get all Main Stars in the Basement", "ranks.c", "GetAll100CoinStars"),
    R("GetAllStarsInFloor1", "B Rank", "Get all Main Stars on Floor 1", "ranks.b", "GetAllStarsInBasement"),
    R("GetAllStarsInFloor2", "A Rank", "Get all Main Stars on Floor 2", "ranks.a", "GetAllStarsInFloor1"),
    R("GetAllStarsInGame", "S Rank", "Get all Main Stars on Floor 3", "ranks.s", "GetAllStarsInFloor2"),
    R("GetAllCastleStars", "S+ Rank", "Get all Castle Main Stars", "ranks.splus", "GetAllStarsInGame"),

    // Boss Achievements
    R("DefeatKingBobomb", "Explosive Test", "Defeat King Bob-omb", "bosses.king-bob"),
    R("DefeatMrI", "I vs Eye", "Defeat Mr. I", "bosses.mr-i"),
    R("DefeatWiggler", "Insecticide", "Defeat Wiggler", "bosses.wiggler"),
    R("DefeatAllBooses", "Boo Who?", "Defeat All Boos in the Haunted House", "bosses.big-boo"),
    R("DefeatAllBigBullies", "The Real Bully", "Defeat All Big Bullies From LLL", "bosses.big-bully"),
    R("DefeatEyerok", "Welcome To The Jam", "Defeat Eyerok", "bosses.eyerok"),
    R("DefeatKingWhomp", "Come On And Slam", "Defeat King Whomp", "bosses.king-whomp"),
    R("DefeatBowser1", "Bowser Trapped In The Dark", "Defeat Bowser in the Dark World", "bosses.bowser1"),
    R("DefeatBowser2", "Bowser Burned By The Lava", "Defeat Bowser in the Fire Sea", "bosses.bowser2"),
    R("DefeatBowser3", "Final Showdown", "Defeat Bowser in the Sky", "bosses.bowser3"),
    R("DefearBowser3WithAllStars", "True Ending", "Defeat Bowser in the Sky with 120 Stars",
      "bosses.bowser3-with-120-stars", "The Completionist"),

    // Death Achievements
    R("DeathByBoss", "Git Gud", "Get Defeated by a Boss", "deaths.boss"),
    R("DeathByFalling", "Gravity Is A Myth", "Die by Falling", "deaths.falling"),
    R("DeathByQuickSand", "Sink Or Swim", "Die in Quick Sand", "deaths.quicksand"),
    R("DeathByCrushing", "Space Jam", "Die by Being Crushed", "deaths.crushing"),
    R("DeathByBowser", "Bad Ending", "Get Defeated by Bowser", "deaths.bowser"),
    R("DeathByEnemy", "Watch Your Step", "Die by an Enemy", "deaths.enemy"),
    R("DeathByDrowning", "Under The Sea", "Die by Drowning", "deaths.drowning"),
    R("DeathByFire", "Roasted Mario", "Die by Fire or Lava", "extras.carpet-burn"),

    // Extra Achievements
    R("TalkWithYoshi", "It Is You?", "Talk with Yoshi on the Roof", "extras.yoshi"),
    P("Slide20Times", "Burned Ass", "Go Down The Slide 20 Times", "extras.carpet-burn", 20),
    R("BeatEveryRace", "Olympic Runner", "Beat Every Racing Challenge", "extras.runner"),
    P("Talk25Times", "Olympic Talker", "Talk to NPCs 25 Times", "extras.talker", 25),
    R("GrabSwimmingStars", "Olympic Swimmer", "Grab every star that needs Metal Cap without it", "extras.swimmer"),
    R("WatchEnding", "The Cake Is A Lie?!", "Watch the game ending", "extras.cake"),
    R("ReleaseChainChomp", "Chomp-Chomp!", "Release the Chain Chomp", "extras.chain-chomp"),
};