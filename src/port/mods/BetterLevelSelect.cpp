#include "BetterLevelSelect.h"

#include "sm64.h"
#include "game/area.h"
#include "audio/external.h"
#include "game/game_init.h"
#include "menu/title_screen.h"
#include "utils/GfxPrint.h"
#include "port/hooks/Events.h"
#include "port/ShipInit.hpp"
#include "geo_commands.h"
#include "gfx_dimensions.h"
#include "assets/levels/intro.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "game/ingame_menu.h"
#include "game/object_list_processor.h"

BetterLevelSelect self;

static const LevelSelectEntry entries[] = {
    {
        .levelId = LEVEL_BOB,
        .englishName = "Bob-omb Battlefield",
        .japaneseName = "ぼぶへいのせんじょう", 
        .actsEn = { "Big Bob-omb on the Summit", "Footrace with Koopa the Quick", "Shoot to the Island in the Sky", "Find the 8 Red Coins", "Mario Wings to the Sky", "Behind Chain Chomp's Gate" },
        .actsJp = { "ぼむきんぐ", "こっぱれーす", "そらのしま", "あかいこいん", "はねまりお", "わんわん" },
    },
    {
        .levelId = LEVEL_WF,
        .englishName = "Whomp's Fortress",
        .japaneseName = "ばったんきんぐのとりで",
        .actsEn = { "Chip Off Whomp's Block", "To the Top of the Fortress", "Shoot into the Wild Blue", "Red Coins on the Floating Isle", "Fall onto the Caged Island", "Blast Away the Wall" },
        .actsJp = { "ばったんきんぐ", "とりでのうえ", "あおいそら", "あかいこいん", "うきじま", "かべのなか" }
    },
    {
        .levelId = LEVEL_JRB,
        .englishName = "Jolly Roger Bay",
        .japaneseName = "かいりきのいりえ",
        .actsEn = { "Plunder in the Sunken Ship", "Can the Eel Come Out to Play?", "Treasure of the Ocean Cave", "Red Coins on the Ship Afloat", "Blast to the Stone Pillar", "Through the Jet Stream" },
        .actsJp = { "ちんぱつせん", "うつぼ", "たからもの", "あかいこいん", "いわのはしら", "じぇっとすとりーむ" },
    },
    {
        .levelId = LEVEL_CCM,
        .englishName = "Cool Cool Mountain", 
        .japaneseName = "さむいさむいまうんてん", 
        .actsEn = { "Slip Slidin' Away", "Li'l Penguin Lost", "Big Penguin Race", "Frosty Slide for 8 Red Coins", "Snowman's Lost His Head", "Wall Kicks Will Work" },
        .actsJp = { "ゆきだるまのめ", "ぺんぎんれーす", "こぺんぎん", "あかいこいん", "ふくろう", "はしをかけろ" },
    },
    {
        .levelId = LEVEL_BBH,
        .englishName = "Big Boo's Haunt", 
        .japaneseName = "てれさのほらーはうす", 
        .actsEn = { "Go on a Ghost Hunt", "Ride Big Boo's Merry-Go-Round", "Secret of the Haunted Books", "Seek the Eight Red Coins", "Big Boo's Balcony Holdup", "Eye to Eye in the Secret Room" },
        .actsJp = { "おばけをたおせ", "めりーごーらんど", "ほんだなのなぞ", "あかいこいん", "ぼるこふのいかり", "ひみつのしつ" },
    },
    {
        .levelId = LEVEL_HMC,
        .englishName = "Hazy Maze Cave",
        .japaneseName = "やみのちかしつ", 
        .actsEn = { "Swimming Beast in the Cavern", "Elevate for 8 Red Coins", "Metal-Head Mario Can Move!", "Navigating the Toxic Maze", "A-Maze-Ing Emergency Exit", "Watch for Rolling Rocks" },
        .actsJp = { "どっしー", "あかいこいん", "めたるまりお", "けむりのなか", "えれべーたー", "ごろごろいわ" },
    },
    {
        .levelId = LEVEL_LLL,
        .englishName = "Lethal Lava Land", 
        .japaneseName = "ぐらぐらかざん", 
        .actsEn = { "Boil the Big Bully", "Bully the Bullies", "8-Coin Puzzle with 15 Pieces", "Red-Hot Log Rolling", "Hot-Foot-It into the Volcano", "Elevator Tour in the Volcano" },
        .actsJp = { "おおどんけつ", "どんけつくん", "あかいこいん", "まるたわたり", "かざんのなか", "えれべーたー" },
    },
    {
        .levelId = LEVEL_SSL,
        .englishName = "Shifting Sand Land",
        .japaneseName = "あついあついさばく",
        .actsEn = { "In the Talons of the Big Bird", "Shining Atop the Pyramid", "Inside the Ancient Pyramid", "Stand Tall on the Four Pillars", "Free Flying for 8 Red Coins", "Pyramid Puzzle" },
        .actsJp = { "とり", "ぴらみっど", "てっぺん", "あかいこいん", "おこるいわ", "なぞのぴらみっど" },
    },
    {
        .levelId = LEVEL_DDD,
        .englishName = "Dire Dire Docks",
        .japaneseName = "みずのぼーどろーど",
        .actsEn = { "Board Bowser's Sub", "Chests in the Current", "Pole-Jumping for Red Coins", "Through the Jet Stream", "The Manta Ray's Reward", "Collect the Caps..." },
        .actsJp = { "せんすいかん", "たからもの", "あかいこいん", "じぇっとすとりーむ", "まんた", "めたるぼう" },
    },
    {
        .levelId = LEVEL_SL,
        .englishName = "Snowman's Land",
        .japaneseName = "しばれるやまのすのーまん", 
        .actsEn = { "Snowman's Big Head", "Chill with the Bully", "In the Deep Freeze", "Whirl from the Freezing Pond", "Shell Shreddin' for Red Coins", "Into the Igloo" },
        .actsJp = { "すのーまんのめ", "どんけつ", "こおりのなか", "ふりーずいけ", "あかいこいん", "いぐるーのなか" },
    },
    {
        .levelId = LEVEL_WDW,
        .englishName = "Wet-Dry World",
        .japaneseName = "みずびたししてぃ",
        .actsEn = { "Shocking Arrow Lifts!", "Top o' the Town", "Secrets in the Shallows & Sky", "Express Elevator--Hurry Up!", "Go to Town for Red Coins", "Quick Race Through Downtown!" },
        .actsJp = { "でんきや", "まちのうえ", "みずのなか", "えれべーたー", "あかいこいん", "だうんたうん" },
    },
    {
        .levelId = LEVEL_TTM,
        .englishName = "Tall Tall Mountain", 
        .japaneseName = "たかいたかいやま", 
        .actsEn = { "Scale the Mountain", "Mystery of the Monkey Cage", "Scary 'Shrooms, Red Coins", "Mysterious Mountainside", "Breathtaking View from Bridge", "Blast to the Lonely Mushroom" },
        .actsJp = { "やまのうえ", "さるのかご", "きのこ", "やまのなか", "はしのうえ", "ひとりぼっちのきのこ" },
    },
    {
        .levelId = LEVEL_THI,
        .englishName = "Tiny-Huge Island",
        .japaneseName = "ちびでかあいらんど",
        .actsEn = { "Huge Island Little Island", "Rematch with Koopa the Quick", "Five Itty Bitty Secrets", "Wiggler's Red Coins", "Make Wiggler Squirm", "Make Wiggler Squirm" },
        .actsJp = { "でかじまちびじま", "こっぱれーす", "いつつのひみつ", "あかいこいん", "はなちゃん", "はなちゃん" },
        .areas = {
            { "Tiny Island", 0x01, 0x0A },
            { "Huge Island", 0x02, 0x0A },
        },
    },
    {
        .levelId = LEVEL_TTC,
        .englishName = "Tick Tock Clock",
        .japaneseName = "ちっくたっくろっく",
        .actsEn = { "Roll into the Cage", "The Pit and the Pendulums", "Get a Hand", "Stomp on the Thwomp", "Timed Jumps on Hollow Steps", "Stop Time for Red Coins" },
        .actsJp = { "かごのなか", "ふりこ", "はり", "どっすん", "ほろーすてっぷ", "あかいこいん" }, 
    },
    {
        .levelId = LEVEL_RR,
        .englishName = "Rainbow Ride",
        .japaneseName = "にじかけるそら",
        .actsEn = { "Cruiser Crossing the Rainbow", "The Big House in the Sky", "Coins Amassed in a Maze", "Swingin' in the Breeze", "Tricky Triangles!", "Somewhere Over the Rainbow" },
        .actsJp = { "にじのふね", "そらのやしき", "めいろのめいろ", "ぶらんこ", "さんかくじょうぎ", "にじをこえて" },
    },
    {
        .levelId = LEVEL_TOTWC,
        .englishName = "Tower of the Wing Cap", 
        .japaneseName = "はねきゃっぷのとう", 
    },
    {
        .levelId = LEVEL_VCUTM,
        .englishName = "Vanish Cap Under the Moat", 
        .japaneseName = "おほりのそこ" 
    },
    {
        .levelId = LEVEL_COTMC,
        .englishName = "Cavern of the Metal Cap", 
        .japaneseName = "めたるきゃっぷのどうくつ", 
    },
    {
        .levelId = LEVEL_PSS,
        .englishName = "Secret Slide", 
        .japaneseName = "ぴーちのかくしすらいど", 
    },
    {
        .levelId = LEVEL_SA,
        .englishName = "The Secret Aquarium", 
        .japaneseName = "かくしすいそう"
    },
    {
        .levelId = LEVEL_WMOTR,
        .englishName = "Wing Mario Over the Rainbow", 
        .japaneseName = "にじをこえたさき", 
    },
    {
        .levelId = LEVEL_BITDW,
        .englishName = "Bowser in the Dark World", 
        .japaneseName = "やみのせかいのくっぱ"
    },
    {
        .levelId = LEVEL_BOWSER_1,
        .englishName = "Bowser in the Dark World Boss", 
        .japaneseName = "くっぱ１", 
    },
    {
        .levelId = LEVEL_BITFS,
        .englishName = "Bowser in the Fire Sea", 
        .japaneseName = "ほのおのうみのくっぱ", 
    },
    {
        .levelId = LEVEL_BOWSER_2,
        .englishName = "Bowser in the Fire Sea Boss", 
        .japaneseName = "くっぱ２", 
    },
    {
        .levelId = LEVEL_BITS,
        .englishName = "Bowser in the Sky", 
        .japaneseName = "てんくうのたたかいのくっぱ", 
    },
    {
        .levelId = LEVEL_BOWSER_3,
        .englishName = "Bowser in the Sky Boss", 
        .japaneseName = "くっぱ３",  
    },
    {
        .levelId = LEVEL_CASTLE_GROUNDS,
        .englishName = "Outside the Castle", 
        .japaneseName = "ぴーちじょうがい"
    },
    {
        .levelId = LEVEL_CASTLE,
        .englishName = "Inside Peach's Castle",
        .japaneseName = "ぴーちじょうない"
    },
    {
        .levelId = LEVEL_CASTLE_COURTYARD,
        .englishName = "Castle Courtyard",
        .japaneseName = "ぴーちじょうなかにわ"
    },
    {
        .levelId = LEVEL_ENDING,
        .englishName = "The End", 
        .japaneseName = "おわり",
    },
};

static const char* ttcSpeeds[] = {
    "Slow",
    "Fast",
    "Random",
    "Stopped"
};

static const char* thiSizes[] = {
    "Tiny", "Huge"
};

extern "C" void initiate_warp(s16 destLevel, s16 destArea, s16 destWarpNode, s32 arg3);

s32 BetterLevelSelect_UpdateArea(s16 a, s32 b) {
    if(self.areaChanged && self.currentAreaIndex > 0) {
        LevelSelectEntry entry = entries[self.currentLevelIndex];
        LevelArea area = entry.areas[self.currentAreaIndex - 1];
        change_area(area.areaId);
        ObjectWarpNode* warpNode = area_get_warp_node(area.warpId);
        initiate_warp(warpNode->node.destLevel & 0x7F, warpNode->node.destArea,
                      warpNode->node.destNode, sDelayedWarpArg);
        level_set_transition(2, NULL);
        self.areaChanged = false;
    }

    return lvl_init_or_update(a, b);
}

s32 BetterLevelSelect_UpdateMenu(s16 arg, s32 b) {
    if(self.forceReload) {
        self.forceReload = false;
        return 1;
    }

    if(arg != LVL_INTRO_LEVEL_SELECT || CVarGetInteger("gDeveloperTools.BetterLevelSelect", 1) == 0){
        return lvl_intro_update(arg, b);
    }

    int count = ARRAY_COUNT(entries);

    gCurrSaveFileNum = 4;

    if(gPlayer1Controller->buttonPressed & BTN_A) {
        play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
        gCurrActNum = self.currentActIndex + 1;
        gDialogCourseActNum = gCurrActNum;
        switch (entries[self.currentLevelIndex].levelId) {
            case LEVEL_TTC:
                gTTCSpeedSetting = self.ttcSpeedIndex;
                break;
            default: break;
        }
        return entries[self.currentLevelIndex].levelId;
    }

    if(gPlayer1Controller->buttonDown & U_JPAD) {
        if (self.lockUp) {
            self.timerUp = 0;
        }
        if (self.timerUp == 0) {
            self.timerUp = 40;
            self.lockUp = true;
            self.verticalInput = self.update_rate;
        }
    }

    if(gPlayer1Controller->buttonDown & D_JPAD) {
        if (self.lockDown) {
            self.timerDown = 0;
        }
        if (self.timerDown == 0) {
            self.timerDown = 40;
            self.lockDown = true;
            self.verticalInput = -self.update_rate;
        }
    }
    
    if (self.verticalInput == 0) {
        self.verticalInputAccumulator = 0;
    }

    if(gPlayer1Controller->buttonPressed & BTN_Z) {
        self.currentActIndex--;
        if(self.currentActIndex < 0) {
            self.currentActIndex = 5;
        }
    }

    if(gPlayer1Controller->buttonPressed & BTN_R) {
        self.currentActIndex++;
        self.currentActIndex %= 6;
    }

    switch (entries[self.currentLevelIndex].levelId) {
        case LEVEL_TTC: {
            if(gPlayer1Controller->buttonPressed & L_JPAD) {
                self.ttcSpeedIndex--;
                if(self.ttcSpeedIndex < 0) {
                    self.ttcSpeedIndex = 3;
                }
            }

            if(gPlayer1Controller->buttonPressed & R_JPAD) {
                self.ttcSpeedIndex++;
                self.ttcSpeedIndex %= 4;
            }
            break;
        }

        default: break;
    }

    if(gPlayer1Controller->buttonPressed & BTN_CLEFT) {
        self.currentAreaIndex--;
        if(self.currentAreaIndex < 0) {
            self.currentAreaIndex = 0;
        }
        self.areaChanged = true;
    }

    if(gPlayer1Controller->buttonPressed & BTN_CRIGHT) {
        self.currentAreaIndex++;
        self.currentAreaIndex %= entries[self.currentLevelIndex].areas.size() + 1;
        self.areaChanged = true;
    }

    self.verticalInputAccumulator += self.verticalInput;

    if(self.verticalInputAccumulator < -7) {
        self.verticalInput = 0;
        self.verticalInputAccumulator = 0;

        self.currentAreaIndex = 0;
        self.currentActIndex = 0;
        self.currentLevelIndex++;
        self.currentLevelIndex = (self.currentLevelIndex + count) % count;
        play_sound(SOUND_GENERAL_LEVEL_SELECT_CHANGE, gGlobalSoundSource);

        if (self.currentLevelIndex == ((self.topDisplayedLevel + count + 19) % count)) {
            self.topDisplayedLevel++;
            self.topDisplayedLevel = (self.topDisplayedLevel + count) % count;
        }
    }

    if(self.verticalInputAccumulator > 7) {
        self.verticalInput = 0;
        self.verticalInputAccumulator = 0;

        if (self.currentLevelIndex == self.topDisplayedLevel) {
            self.topDisplayedLevel -= 2;
            self.topDisplayedLevel = (self.topDisplayedLevel + count) % count;
        }

        self.currentAreaIndex = 0;
        self.currentActIndex = 0;
        self.currentLevelIndex--;
        self.currentLevelIndex = (self.currentLevelIndex + count) % count;
        play_sound(SOUND_GENERAL_LEVEL_SELECT_CHANGE, gGlobalSoundSource);

        if (self.currentLevelIndex == ((self.topDisplayedLevel + count) % count)) {
            self.topDisplayedLevel--;
            self.topDisplayedLevel = (self.topDisplayedLevel + count) % count;
        }
    }

    self.currentLevelIndex = (self.currentLevelIndex + count) % count;
    self.topDisplayedLevel = (self.topDisplayedLevel + count) % count;

    if(self.timerUp != 0) {
        self.timerUp--;
    }

    if(self.timerUp == 0){
        self.lockUp = false;
    }

    if(self.timerDown != 0) {
        self.timerDown--;
    }

    if(self.timerDown == 0){
        self.lockDown = false;
    }
    return 0;
}

Gfx* BetterLevelSelect_DrawMenu(s32 state, struct GraphNode *node, UNUSED void *context) {
    if(state != 1 || CVarGetInteger("gDeveloperTools.BetterLevelSelect", 1) != 1) {
        return NULL;
    }

    int32_t count = ARRAY_COUNT(entries);
    GfxPrint printer;
    Gfx* head = &self.pool[0];
    gDPSetRenderMode(head++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(head++, G_CYC_FILL);
    gDPSetFillColor(head++, 0x0001);
    gDPFillWideRectangle(head++,
                     GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0), BORDER_HEIGHT,
                     GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0) - 1, SCREEN_HEIGHT - BORDER_HEIGHT - 1);
    gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);

    GfxPrint_Init(&printer);

    GfxPrint_Open(&printer, head);
        GfxPrint_SetColor(&printer, 255, 255, 255, 255);
        GfxPrint_SetPos(&printer, 12, 2);
        GfxPrint_Printf(&printer, "Scene Selection");

        for(int i = 0; i < 20; i++) {
            int idx = (self.topDisplayedLevel + i + count) % count;
            LevelSelectEntry entry = entries[idx];
            GfxPrint_SetPos(&printer, 3, i + 4);

            if(idx == self.currentLevelIndex) {
                GfxPrint_SetColor(&printer, 255, 100, 100, 255);
            } else {
                GfxPrint_SetColor(&printer, 175, 175, 175, 255);
            }

            GfxPrint_Printf(&printer, "%3d %s", idx, ROM_JP ? entry.japaneseName : entry.englishName);
        }

        std::vector<const char*> acts = ROM_JP ? entries[self.currentLevelIndex].actsJp : entries[self.currentLevelIndex].actsEn;

        if(!acts.empty()) {
            GfxPrint_SetPos(&printer, 2, 25);
            GfxPrint_SetColor(&printer, 100, 100, 100, 255);
            GfxPrint_Printf(&printer, "(Z/R)Act:");
            GfxPrint_SetColor(&printer, 200, 200, 50, 255);
            GfxPrint_Printf(&printer, "%s", acts[self.currentActIndex]);

            GfxPrint_SetPos(&printer, 2, 26);
            auto areas = entries[self.currentLevelIndex].areas;
            GfxPrint_SetColor(&printer, 100, 100, 100, 255);
            GfxPrint_Printf(&printer, "Area:");
            GfxPrint_SetColor(&printer, 200, 50, 50, 255);
            GfxPrint_Printf(&printer, "%s", self.currentAreaIndex == 0 ? "Default" : areas[self.currentAreaIndex - 1].name);

            GfxPrint_SetPos(&printer, 2, 27);
            GfxPrint_SetColor(&printer, 100, 100, 100, 255);

            switch (entries[self.currentLevelIndex].levelId) {
                // case LEVEL_WDW: {
                //     GfxPrint_Printf(&printer, "Water Level:");
                //     GfxPrint_SetColor(&printer, 55, 200, 50, 255);
                //     GfxPrint_Printf(&printer, "%s", ttcSpeeds[self.ttcSpeedIndex]);
                //     break;
                // }
                case LEVEL_TTC: {
                    GfxPrint_Printf(&printer, "Speed:");
                    GfxPrint_SetColor(&printer, 55, 200, 50, 255);
                    GfxPrint_Printf(&printer, "%s", ttcSpeeds[self.ttcSpeedIndex]);
                    break;
                }
                default: break;
            }
        }

    head = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
    gSPEndDisplayList(head);
    return self.pool;
}

static const GeoLayout BetterLevelSelect_GeoWrapper[] = {
    GEO_NODE_SCREEN_AREA(0, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, SCREEN_WIDTH/2, SCREEN_HEIGHT/2),
    GEO_OPEN_NODE(),
        GEO_ZBUFFER(0),
        GEO_OPEN_NODE(),
            GEO_NODE_ORTHO(100),
            GEO_OPEN_NODE(),
                GEO_ASM(0, BetterLevelSelect_DrawMenu),
            GEO_CLOSE_NODE(),
        GEO_CLOSE_NODE(),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

static void Init() {
    REGISTER_LISTENER(LevelScriptBeginArea, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptBeginArea* ev = (LevelScriptBeginArea*) event;

        if (strcmp((char*) *ev->geoLayoutAddr, intro_geo_000414) != 0) {
            return;
        }

        if(CVarGetInteger("gDeveloperTools.BetterLevelSelect", 1) == 0){
            *ev->geoLayoutAddr = (void*) intro_geo_000414;
        } else {
            *ev->geoLayoutAddr = (void*) BetterLevelSelect_GeoWrapper;
        }
    });

    REGISTER_LISTENER(LevelScriptCallLoop, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptCallLoop* ev = (LevelScriptCallLoop*) event;

        if(*ev->func == lvl_intro_update) {
            if(*ev->arg == LVL_INTRO_LEVEL_SELECT) {
                self.loaded = true;
            }
            *ev->func = BetterLevelSelect_UpdateMenu;
            return;
        }

        self.loaded = false;

        if(*ev->func == lvl_init_or_update) {
            *ev->func = BetterLevelSelect_UpdateArea;
            return;
        }
    });
}

void BetterLevelSelect_HandleReload() {
    if(!self.loaded){
        return;
    }
    self.forceReload = true;
}

static RegisterShipInitFunc initFunc(Init);