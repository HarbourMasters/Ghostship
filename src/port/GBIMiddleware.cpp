#include <libultraship.h>

#include "Engine.h"
#include <fast/resource/ResourceType.h>
#include <fast/resource/type/DisplayList.h>

typedef struct {
    int index;
    Gfx instruction;
} GfxPatch;

std::unordered_map<std::string, std::unordered_map<std::string, GfxPatch>> originalGfx;

// Attention! This is primarily for cosmetics & bug fixes. For things like mods and model replacement you should be
// using OTRs instead (When that is available). Index can be found using the commented out section below.
extern "C" void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path));

    // Leaving this here for people attempting to find the correct Dlist index to patch
    /*if (strcmp("__OTR__objects/object_gi_longsword/gGiBiggoronSwordDL", path) == 0) {
        for (int i = 0; i < res->instructions.size(); i++) {
            Gfx* gfx = (Gfx*)&res->instructions[i];
            // Log all commands
            // SPDLOG_INFO("index:{} command:{}", i, gfx->words.w0 >> 24);
            // Log only SetPrimColors
            if (gfx->words.w0 >> 24 == 250) {
                SPDLOG_INFO("index:{} r:{} g:{} b:{} a:{}", i, _SHIFTR(gfx->words.w1, 24, 8), _SHIFTR(gfx->words.w1, 16,
    8), _SHIFTR(gfx->words.w1, 8, 8), _SHIFTR(gfx->words.w1, 0, 8));
            }
        }
    }*/

    // Index refers to individual gfx words, which are half the size on 32-bit
    // if (sizeof(uintptr_t) < 8) {
    // index /= 2;
    // }

    // Do not patch custom assets as they most likely do not have the same instructions as authentic assets
    if (res->GetInitData()->IsCustom) {
        return;
    }

    Gfx* gfx = (Gfx*)&res->Instructions[index];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { index, *gfx };
    }

    *gfx = instruction;
}

extern "C" void gSPDisplayList(Gfx* pkt, Gfx* dl) {
    char* imgData = (char*)dl;

    if (GameEngine_OTRSigCheck(imgData) == 1) {
        auto resource = Ship::Context::GetInstance()->GetResourceManager()->LoadResource(imgData);
        auto res = std::static_pointer_cast<Fast::DisplayList>(resource);
        dl = &res->Instructions[0];
#ifdef USE_GBI_TRACE
        printf("DisplayList: %s\n", imgData);
        for (int i = 0; i < res->Instructions.size(); i++) {
            auto gfx = &res->Instructions[i];
            gfx->words.trace.file = imgData;
            gfx->words.trace.idx = i;
            gfx->words.trace.valid = 0xB00B;
        }
#endif
    }

    __gSPDisplayList(pkt, dl);
}

extern "C" void gSPVertex(Gfx* pkt, uintptr_t v, int n, int v0) {

    if (GameEngine_OTRSigCheck((char*)v) == 1) {
        v = (uintptr_t) ResourceGetDataByName((char *) v);
    }

    __gSPVertex(pkt, v, n, v0);
}

extern "C" void gSPInvalidateTexCache(Gfx* pkt, uintptr_t texAddr) {
    char* imgData = (char*)texAddr;

    if (texAddr != 0 && GameEngine_OTRSigCheck(imgData)) {
        auto res = Ship::Context::GetInstance()->GetResourceManager()->LoadResource(imgData);

        if (res->GetInitData()->Type == (uint32_t) Fast::ResourceType::DisplayList)
            texAddr = (uintptr_t)&((std::static_pointer_cast<Fast::DisplayList>(res))->Instructions[0]);
        else {
            texAddr = (uintptr_t) res->GetRawPointer();
        }
    }
    __gSPInvalidateTexCache(pkt, texAddr);
}