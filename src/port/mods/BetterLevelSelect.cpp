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
#include "engine/level_script.h"
#include "game/ingame_menu.h"
#include "game/object_list_processor.h"

BetterLevelSelect self;

static const LevelSelectEntry entries[] = {
    { 
        "Big Boo's Haunt", "てれさのほらーはうす", 
        { "Go on a Ghost Hunt", "Ride Big Boo's Merry-Go-Round", "Secret of the Haunted Books", "Seek the Eight Red Coins", "Big Boo's Balcony Holdup", "Eye to Eye in the Secret Room" },
        { "おばけをたおせ", "めりーごーらんど", "ほんだなのなぞ", "あかいこいん", "ぼるこふのいかり", "ひみつのしつ" },
        LEVEL_BBH 
    },
    { 
        "Cool Cool Mountain", "さむいさむいまうんてん", 
        { "Slip Slidin' Away", "Li'l Penguin Lost", "Big Penguin Race", "Frosty Slide for 8 Red Coins", "Snowman's Lost His Head", "Wall Kicks Will Work" },
        { "ゆきだるまのめ", "ぺんぎんれーす", "こぺんぎん", "あかいこいん", "ふくろう", "はしをかけろ" },
        LEVEL_CCM 
    },
    { "Inside Peach's Castle", "ぴーちじょうない", { NULL }, { NULL }, LEVEL_CASTLE },
    { 
        "Hazy Maze Cave", "やみのちかしつ", 
        { "Swimming Beast in the Cavern", "Elevate for 8 Red Coins", "Metal-Head Mario Can Move!", "Navigating the Toxic Maze", "A-Maze-Ing Emergency Exit", "Watch for Rolling Rocks" },
        { "どっしー", "あかいこいん", "めたるまりお", "けむりのなか", "えれべーたー", "ごろごろいわ" },
        LEVEL_HMC 
    },
    { 
        "Shifting Sand Land", "あついあついさばく", 
        { "In the Talons of the Big Bird", "Shining Atop the Pyramid", "Inside the Ancient Pyramid", "Stand Tall on the Four Pillars", "Free Flying for 8 Red Coins", "Pyramid Puzzle" },
        { "とり", "ぴらみっど", "てっぺん", "あかいこいん", "おこるいわ", "なぞのぴらみっど" },
        LEVEL_SSL 
    },
    { 
        "Bob-omb Battlefield", "ぼぶへいのせんじょう", 
        { "Big Bob-omb on the Summit", "Footrace with Koopa the Quick", "Shoot to the Island in the Sky", "Find the 8 Red Coins", "Mario Wings to the Sky", "Behind Chain Chomp's Gate" },
        { "ぼむきんぐ", "こっぱれーす", "そらのしま", "あかいこいん", "はねまりお", "わんわん" },
        LEVEL_BOB 
    },
    { 
        "Snowman's Land", "しばれるやまのすのーまん", 
        { "Snowman's Big Head", "Chill with the Bully", "In the Deep Freeze", "Whirl from the Freezing Pond", "Shell Shreddin' for Red Coins", "Into the Igloo" },
        { "すのーまんのめ", "どんけつ", "こおりのなか", "ふりーずいけ", "あかいこいん", "いぐるーのなか" },
        LEVEL_SL 
    },
    { 
        "Wet-Dry World", "みずびたししてぃ", 
        { "Shocking Arrow Lifts!", "Top o' the Town", "Secrets in the Shallows & Sky", "Express Elevator--Hurry Up!", "Go to Town for Red Coins", "Quick Race Through Downtown!" },
        { "でんきや", "まちのうえ", "みずのなか", "えれべーたー", "あかいこいん", "だうんたうん" },
        LEVEL_WDW 
    },
    { 
        "Jolly Roger Bay", "かいりきのいりえ", 
        { "Plunder in the Sunken Ship", "Can the Eel Come Out to Play?", "Treasure of the Ocean Cave", "Red Coins on the Ship Afloat", "Blast to the Stone Pillar", "Through the Jet Stream" },
        { "ちんぱつせん", "うつぼ", "たからもの", "あかいこいん", "いわのはしら", "じぇっとすとりーむ" },
        LEVEL_JRB 
    },
    { 
        "Tiny-Huge Island", "ちびでかあいらんど", 
        { "Huge Island Little Island", "Rematch with Koopa the Quick", "Five Itty Bitty Secrets", "Wiggler's Red Coins", "Make Wiggler Squirm", "Make Wiggler Squirm" },
        { "でかじまちびじま", "こっぱれーす", "いつつのひみつ", "あかいこいん", "はなちゃん", "はなちゃん" },
        LEVEL_THI 
    },
    { 
        "Tick Tock Clock", "ちっくたっくろっく", 
        { "Roll into the Cage", "The Pit and the Pendulums", "Get a Hand", "Stomp on the Thwomp", "Timed Jumps on Hollow Steps", "Stop Time for Red Coins" },
        { "かごのなか", "ふりこ", "はり", "どっすん", "ほろーすてっぷ", "あかいこいん" },
        LEVEL_TTC 
    },
    { 
        "Rainbow Ride", "にじかけるそら", 
        { "Cruiser Crossing the Rainbow", "The Big House in the Sky", "Coins Amassed in a Maze", "Swingin' in the Breeze", "Tricky Triangles!", "Somewhere Over the Rainbow" },
        { "にじのふね", "そらのやしき", "めいろのめいろ", "ぶらんこ", "さんかくじょうぎ", "にじをこえて" },
        LEVEL_RR 
    },
    { "Outside the Castle", "ぴーちじょうがい", { NULL }, { NULL }, LEVEL_CASTLE_GROUNDS },
    { "Bowser in the Dark World", "やみのせかいのくっぱ", { NULL }, { NULL }, LEVEL_BITDW },
    { "Vanish Cap Under the Moat", "おほりのそこ", { NULL }, { NULL }, LEVEL_VCUTM },
    { "Bowser in the Fire Sea", "ほのおのうみのくっぱ", { NULL }, { NULL }, LEVEL_BITFS },
    { "The Secret Aquarium", "かくしすいそう", { NULL }, { NULL }, LEVEL_SA },
    { "Bowser in the Sky", "てんくうのたたかいのくっぱ", { NULL }, { NULL }, LEVEL_BITS },
    { 
        "Lethal Lava Land", "ぐらぐらかざん", 
        { "Boil the Big Bully", "Bully the Bullies", "8-Coin Puzzle with 15 Pieces", "Red-Hot Log Rolling", "Hot-Foot-It into the Volcano", "Elevator Tour in the Volcano" },
        { "おおどんけつ", "どんけつくん", "あかいこいん", "まるたわたり", "かざんのなか", "えれべーたー" },
        LEVEL_LLL 
    },
    { 
        "Dire Dire Docks", "みずのぼーどろーど", 
        { "Board Bowser's Sub", "Chests in the Current", "Pole-Jumping for Red Coins", "Through the Jet Stream", "The Manta Ray's Reward", "Collect the Caps..." },
        { "せんすいかん", "たからもの", "あかいこいん", "じぇっとすとりーむ", "まんた", "めたるぼう" },
        LEVEL_DDD 
    },
    { 
        "Whomp's Fortress", "ばったんきんぐのとりで", 
        { "Chip Off Whomp's Block", "To the Top of the Fortress", "Shoot into the Wild Blue", "Red Coins on the Floating Isle", "Fall onto the Caged Island", "Blast Away the Wall" },
        { "ばったんきんぐ", "とりでのうえ", "あおいそら", "あかいこいん", "うきじま", "かべのなか" },
        LEVEL_WF 
    },
    { "The End", "おわり", { NULL }, { NULL }, LEVEL_ENDING },
    { "Castle Courtyard", "ぴーちじょうなかにわ", { NULL }, { NULL }, LEVEL_CASTLE_COURTYARD },
    { "Secret Slide", "ぴーちのかくしすらいど", { NULL }, { NULL }, LEVEL_PSS },
    { "Cavern of the Metal Cap", "めたるきゃっぷのどうくつ", { NULL }, { NULL }, LEVEL_COTMC },
    { "Tower of the Wing Cap", "はねきゃっぷのとう", { NULL }, { NULL }, LEVEL_TOTWC },
    { "Bowser in the Dark World Boss", "くっぱ１", { NULL }, { NULL }, LEVEL_BOWSER_1 },
    { "Wing Mario Over the Rainbow", "にじをこえたさき", { NULL }, { NULL }, LEVEL_WMOTR },
    { "Bowser in the Fire Sea Boss", "くっぱ２", { NULL }, { NULL }, LEVEL_BOWSER_2 },
    { "Bowser in the Sky Boss", "くっぱ３", { NULL }, { NULL }, LEVEL_BOWSER_3 },
    { 
        "Tall Tall Mountain", "たかいたかいやま", 
        { "Scale the Mountain", "Mystery of the Monkey Cage", "Scary 'Shrooms, Red Coins", "Mysterious Mountainside", "Breathtaking View from Bridge", "Blast to the Lonely Mushroom" },
        { "やまのうえ", "さるのかご", "きのこ", "やまのなか", "はしのうえ", "ひとりぼっちのきのこ" },
        LEVEL_TTM 
    },
};

static const char* ttcSpeeds[] = {
    "Slow",
    "Fast",
    "Random",
    "Stopped"
};

s32 BetterLevelSelect_UpdateMenu(s16 a, s32 b) {
    if(self.forceReload) {
        self.forceReload = false;
        return 1;
    }

    if(CVarGetInteger("gDeveloperTools.BetterLevelSelect", 0) == 0){
        return lvl_intro_update(a, b);
    }

    int count = ARRAY_COUNT(entries);

    gCurrSaveFileNum = 4;

    if(gPlayer1Controller->buttonPressed & BTN_A) {
        play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
        gCurrActNum = self.currentActIndex + 1;
        gDialogCourseActNum = gCurrActNum;
        switch (entries[self.currentLevelIndex].areaId) {
            case LEVEL_TTC:
                gTTCSpeedSetting = self.ttcSpeedIndex;
                break;
            default: break;
        }
        return entries[self.currentLevelIndex].areaId;
    }

    if(gPlayer1Controller->buttonDown & U_JPAD) {
        if (self.lockUp) {
            self.timerUp = 0;
        }
        if (self.timerUp == 0) {
            self.timerUp = 20;
            self.lockUp = true;
            play_sound(SOUND_GENERAL_LEVEL_SELECT_CHANGE, gGlobalSoundSource);
            self.verticalInput = self.update_rate;
        }
    }

    if(gPlayer1Controller->buttonDown & U_JPAD && self.timerUp == 0) {
        self.verticalInput = self.update_rate * 3;
    }

    if(gPlayer1Controller->buttonDown & D_JPAD) {
        if (self.lockDown) {
            self.timerDown = 0;
        }
        if (self.timerDown == 0) {
            self.timerDown = 20;
            self.lockDown = true;
            play_sound(SOUND_GENERAL_LEVEL_SELECT_CHANGE, gGlobalSoundSource);
            self.verticalInput = -self.update_rate;
        }
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

    switch (entries[self.currentLevelIndex].areaId) {
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

    if(gPlayer1Controller->buttonDown & D_JPAD && self.timerDown == 0) {
        self.verticalInput = -self.update_rate * 3;
    }

    self.verticalInputAccumulator += self.verticalInput;

    if(self.verticalInputAccumulator < -7) {
        self.verticalInput = 0;
        self.verticalInputAccumulator = 0;

        self.currentActIndex = 0;
        self.currentLevelIndex++;
        self.currentLevelIndex = (self.currentLevelIndex + count) % count;

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

        self.currentActIndex = 0;
        self.currentLevelIndex--;
        self.currentLevelIndex = (self.currentLevelIndex + count) % count;

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
    if(state != 1 || CVarGetInteger("gDeveloperTools.BetterLevelSelect", 0) != 1) {
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

    const char* const *acts = ROM_JP ? entries[self.currentLevelIndex].actsJp : entries[self.currentLevelIndex].actsEn;
    if(acts[0] != NULL) {
        GfxPrint_SetPos(&printer, 1, 25);
        GfxPrint_SetColor(&printer, 100, 100, 100, 255);
        GfxPrint_Printf(&printer, "(Z/R)Act:");
        GfxPrint_SetColor(&printer, 200, 200, 50, 255);
        GfxPrint_Printf(&printer, "%s", acts[self.currentActIndex]);

        GfxPrint_SetPos(&printer, 1, 26);
        GfxPrint_SetColor(&printer, 100, 100, 100, 255);
        switch (entries[self.currentLevelIndex].areaId) {
            case LEVEL_WDW: {
                GfxPrint_Printf(&printer, "Water Level:");
                GfxPrint_SetColor(&printer, 55, 200, 50, 255);
                GfxPrint_Printf(&printer, "%s", ttcSpeeds[self.ttcSpeedIndex]);
                break;
            }
            case LEVEL_TTC: {
                GfxPrint_Printf(&printer, "Speed:");
                GfxPrint_SetColor(&printer, 55, 200, 50, 255);
                GfxPrint_Printf(&printer, "%s", ttcSpeeds[self.ttcSpeedIndex]);
                break;
            }
            case LEVEL_THI: {
                GfxPrint_Printf(&printer, "Size:");
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

        if(CVarGetInteger("gDeveloperTools.BetterLevelSelect", 0) == 0){
            *ev->geoLayoutAddr = (void*) intro_geo_000414;
        } else {
            *ev->geoLayoutAddr = (void*) BetterLevelSelect_GeoWrapper;
        }
    });

    REGISTER_LISTENER(LevelScriptCallLoop, EVENT_PRIORITY_NORMAL, [](IEvent* event) {
        LevelScriptCallLoop* ev = (LevelScriptCallLoop*) event;

        if(*ev->func != lvl_intro_update || *ev->arg != LVL_INTRO_LEVEL_SELECT) {
            self.loaded = false;
            return;
        }

        *ev->func = BetterLevelSelect_UpdateMenu;
        self.loaded = true;
    });
}

void BetterLevelSelect_HandleReload() {
    if(!self.loaded){
        return;
    }
    self.forceReload = true;
}

static RegisterShipInitFunc initFunc(Init);