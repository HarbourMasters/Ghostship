#include "ShipUtils.h"
#include <libultraship/libultraship.h>
#include "fast/Fast3dGui.h"

#include "sm64.h"
#include "macros.h"
#include "include/assets/textures/segment2.h"
#include "include/level_table.h"
#include "include/course_table.h"

constexpr f32 fourByThree = 4.0f / 3.0f;

extern "C" bool Ship_IsCStringEmpty(const char* str) {
    return str == NULL || str[0] == '\0';
}

char seedString[MAX_SEED_STRING_SIZE];
u32 finalSeed = 0;

extern uint32_t Ship_Hash(std::string str) {
    // FNV-1a
    const size_t len = str.size();
    uint32_t hval = 0x811c9dc5;
    for (size_t pos = 0; pos < len; pos++) {
        hval ^= (uint32_t)str[pos];
        hval *= 0x01000193;
    }
    return hval;
}

std::vector<std::string> levelAbbreviations = {
    "BOB", "WF",  "JRB", "CCM",   "BBH",   "HMC",  "LLL", "SSL",   "DDD",   "SL",    "WDW",   "TTM",
    "THI", "TTC", "RR",  "BITDW", "BITFS", "BITS", "PSS", "COTMC", "TOTWC", "VCUTM", "WMOTR", "SA",
};

std::map<int16_t, std::string> levelIdList = {
    { LEVEL_BOB, "Bob Omb Battlefield" },
    { LEVEL_WF, "Whomp's Fortress" },
    { LEVEL_JRB, "Jolly Roger's Bay" },
    { LEVEL_CASTLE, "Castle Interior" },
    { LEVEL_CCM, "Cool Cool Mountain" },
    { LEVEL_BBH, "Big Boo's Haunt" },
    { LEVEL_HMC, "Hazy Maze Cave" },
    { LEVEL_LLL, "Lethal Lava Land" },
    { LEVEL_SSL, "Shifting Sand Land" },
    { LEVEL_DDD, "Dire Dire Docks" },
    { LEVEL_SL, "Snowman's Land" },
    { LEVEL_WDW, "Wet Dry World" },
    { LEVEL_TTM, "Tall Tall Mountain" },
    { LEVEL_THI, "Tiny Huge Island" },
    { LEVEL_TTC, "Tick Tock Clock" },
    { LEVEL_RR, "Rainbow Ride" },
    { LEVEL_BITDW, "Bowser in the Dark World" },
    { LEVEL_BITFS, "Bowser in the Fire Sea" },
    { LEVEL_BITS, "Bowser in the Sky" },
    { LEVEL_PSS, "Princess's Secret Slide" },
    { LEVEL_COTMC, "Cavern of the Metal Cap" },
    { LEVEL_TOTWC, "Tower of the Wing Cap" },
    { LEVEL_VCUTM, "Vanish Cap Under the Moat" },
    { LEVEL_WMOTR, "Wing Mario over the Rainbow" },
    { LEVEL_SA, "Secret Aquarium" },
};

std::map<int16_t, int16_t> levelToCourseMap = {
    { LEVEL_BBH, COURSE_BBH },     { LEVEL_CASTLE, COURSE_NONE }, { LEVEL_CCM, COURSE_CCM },
    { LEVEL_HMC, COURSE_HMC },     { LEVEL_SSL, COURSE_SSL },     { LEVEL_BOB, COURSE_BOB },
    { LEVEL_SL, COURSE_SL },       { LEVEL_WDW, COURSE_WDW },     { LEVEL_JRB, COURSE_JRB },
    { LEVEL_THI, COURSE_THI },     { LEVEL_TTC, COURSE_TTC },     { LEVEL_RR, COURSE_RR },
    { LEVEL_BITDW, COURSE_BITDW }, { LEVEL_VCUTM, COURSE_VCUTM }, { LEVEL_BITFS, COURSE_BITFS },
    { LEVEL_SA, COURSE_SA },       { LEVEL_BITS, COURSE_BITS },   { LEVEL_LLL, COURSE_LLL },
    { LEVEL_DDD, COURSE_DDD },     { LEVEL_WF, COURSE_WF },       { LEVEL_PSS, COURSE_PSS },
    { LEVEL_COTMC, COURSE_COTMC }, { LEVEL_TOTWC, COURSE_TOTWC }, { LEVEL_WMOTR, COURSE_WMOTR },
    { LEVEL_TTM, COURSE_TTM },
};

int16_t Ship_GetCourseByLevel(int16_t levelId) {
    if (levelToCourseMap.find(levelId) != levelToCourseMap.end()) {
        return levelToCourseMap.at(levelId);
    }
    return COURSE_NONE;
}

// Build vertex coordinates for a quad command
// In order of top left, top right, bottom left, then bottom right
// Supports flipping the texture horizontally
extern "C" void Ship_CreateQuadVertexGroup(Vtx* vtxList, s32 xStart, s32 yStart, s32 width, s32 height, u8 flippedH) {
    vtxList[0].v.ob[0] = xStart;
    vtxList[0].v.ob[1] = yStart;
    vtxList[0].v.tc[0] = (flippedH ? width : 0) << 5;
    vtxList[0].v.tc[1] = 0 << 5;

    vtxList[1].v.ob[0] = xStart + width;
    vtxList[1].v.ob[1] = yStart;
    vtxList[1].v.tc[0] = (flippedH ? width * 2 : width) << 5;
    vtxList[1].v.tc[1] = 0 << 5;

    vtxList[2].v.ob[0] = xStart;
    vtxList[2].v.ob[1] = yStart + height;
    vtxList[2].v.tc[0] = (flippedH ? width : 0) << 5;
    vtxList[2].v.tc[1] = height << 5;

    vtxList[3].v.ob[0] = xStart + width;
    vtxList[3].v.ob[1] = yStart + height;
    vtxList[3].v.tc[0] = (flippedH ? width * 2 : width) << 5;
    vtxList[3].v.tc[1] = height << 5;
}

std::string convertEnumToReadableName(const std::string& input) {
    std::string result;
    std::string content = input;

    // Step 1: Remove "RC_" prefix if present
    const std::string prefix = "RC_";
    if (content.rfind(prefix, 0) == 0) {
        content = content.substr(prefix.size());
    }

    // Step 2: Remove level abbreviation if present
    for (auto& abbr : levelAbbreviations) {
        std::string prefix = abbr + "_";
        if (content.rfind(prefix, 0) == 0) {
            content = content.substr(prefix.size());
            break;
        }
    }

    // Step 3: Split the string by '_'
    std::vector<std::string> words;
    std::string word;
    std::istringstream stream(content);
    while (std::getline(stream, word, '_')) {
        words.push_back(word);
    }

    // Step 4: Capitalize the first letter of each word
    for (auto& w : words) {
        std::transform(w.begin(), w.end(), w.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!w.empty()) {
            if (w == "hp") {
                w = "HP";
            } else {
                w[0] = std::toupper(w[0]);
            }
        }
    }

    // Step 5: Join the words with spaces
    for (size_t i = 0; i < words.size(); ++i) {
        result += words[i];
        if (i < words.size() - 1) {
            result += " ";
        }
    }

    return result;
}

std::array<const char*, 3> miscellaneousTextures = {
    texture_hud_char_star,
    texture_hud_char_mario_head,
    texture_hud_char_coin,
};

std::array<const char*, 10> digitList = { texture_hud_char_0, texture_hud_char_1, texture_hud_char_2,
                                          texture_hud_char_3, texture_hud_char_4, texture_hud_char_5,
                                          texture_hud_char_6, texture_hud_char_7, texture_hud_char_8,
                                          texture_hud_char_9 };

void LoadGuiTextures() {
    auto gui = std::static_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetInstance()->GetWindow()->GetGui());
    for (const auto entry : miscellaneousTextures) {
        gui->LoadGuiTexture(entry, entry, ImVec4(1, 1, 1, 1));
    }
    for (const auto entry : digitList) {
        gui->LoadGuiTexture(entry, entry, ImVec4(1, 1, 1, 1));
    }

    gui->LoadGuiTexture("Red Coin Icon", texture_hud_char_coin, ImVec4(1, 0, 0, 1));
    gui->LoadGuiTexture("Blue Coin Icon", texture_hud_char_coin, ImVec4(0.20f, 0.20f, 1, 1));
}
