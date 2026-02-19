#ifndef SHIP_UTILS_H
#define SHIP_UTILS_H

#include <libultraship/libultraship.h>

#ifdef __cplusplus

#define WIDGET_COLOR UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5))

#define MAX_SEED_STRING_SIZE 1024
extern char seedString[MAX_SEED_STRING_SIZE];
extern u32 finalSeed;

uint32_t Ship_Hash(std::string str);

void LoadGuiTextures();
extern int16_t Ship_GetCourseByLevel(int16_t levelId);
extern std::array<const char*, 10> digitList;
extern std::map<int16_t, std::string> levelIdList;
extern std::string convertEnumToReadableName(const std::string& input);

extern "C" {
#endif

bool Ship_IsCStringEmpty(const char* str);

#ifdef __cplusplus
}
#endif

#endif // SHIP_UTILS_H
