#pragma once

#include "game/save_file.h"

#define CALL(func, ...) \
    func(__VA_ARGS__);  \
    return;

extern_s void RestoreMainMenuData(int32_t srcSlot);
extern_s void RestoreSaveFileData(int32_t fileIndex, int32_t srcSlot);
extern_s void SaveFileDoSave(int32_t fileIndex);
extern_s void SaveFileLoadAll(void);
extern_s void SaveMainMenuData(void);
extern_s bool ShouldLoadOldSaveFile(void);