#pragma once

#include "game/save_file.h"

#define CALL(func, ...) \
    func(__VA_ARGS__);  \
    return;

#ifdef __cplusplus
extern "C" {
#endif

void RestoreMainMenuData(int32_t srcSlot);
void RestoreSaveFileData(int32_t fileIndex, int32_t srcSlot);
void SaveFileDoSave(int32_t fileIndex);
void SaveFileLoadAll(void);
void SaveMainMenuData(void);
bool ShouldLoadOldSaveFile(void);

#ifdef __cplusplus
}
#endif