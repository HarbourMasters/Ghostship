#pragma once
#ifdef __EMSCRIPTEN__

#include <string>

void WebCache_Mount(const char* path);
void WebCache_Load();
void WebCache_Save();

std::string WebFilePicker_PickROM();
bool WebFilePicker_PickInto(const char* accept, const char* destPath);

#endif // __EMSCRIPTEN__
