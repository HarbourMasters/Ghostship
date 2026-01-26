#ifndef SHIP_UTILS_H
#define SHIP_UTILS_H

#include <libultraship/libultraship.h>

#ifdef __cplusplus

void LoadGuiTextures();

extern "C" {
#endif

bool Ship_IsCStringEmpty(const char* str);
std::string convertEnumToReadableName(const std::string& input);
int16_t Ship_GetCourseByLevel(int16_t levelId);

#ifdef __cplusplus
}
#endif

#endif // SHIP_UTILS_H
