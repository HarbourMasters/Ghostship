#pragma once

#ifdef __cplusplus

#include <ship/utils/binarytools/BinaryReader.h>

class GeoLayoutParser {
public:
    static Ship::BinaryReader* mReader;
    static void execute(const char* path);
};
#else
void GeoLayoutExecute(char const* path);
#endif
