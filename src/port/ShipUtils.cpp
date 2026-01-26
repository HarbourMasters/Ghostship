#include "ShipUtils.h"
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
#include "include/assets/textures/segment2.h"
}

constexpr f32 fourByThree = 4.0f / 3.0f;

extern "C" bool Ship_IsCStringEmpty(const char* str) {
    return str == NULL || str[0] == '\0';
}

std::vector<std::string> levelAbbreviations = {
    "BOB", "WF",  "JRB", "CCM",   "BBH",   "HMC",  "LLL", "SSL",   "DDD",   "SL",    "WDW",   "TTM",
    "THI", "TTC", "RR",  "BITDW", "BITFS", "BITS", "PSS", "COTMC", "TOTWC", "VCUTM", "WMOTR", "SA",
};

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

void LoadGuiTextures() {
    for (const auto entry : miscellaneousTextures) {
        Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadGuiTexture(entry, entry, ImVec4(1, 1, 1, 1));
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadGuiTexture("Red Coin Icon", texture_hud_char_coin,
                                                                        ImVec4(1, 0, 0, 1));
}
