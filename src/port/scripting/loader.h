#pragma once

#include <string>
#include <vector>

typedef void (*ModFunc_t)(void);
typedef void* ModHandle_t;

class ModInstance {
public:
    ModInstance() : mHandle(nullptr), mTempFile("") {}

    std::string GenerateTempFile();
    void Init(const std::string& path);
    ModFunc_t GetFunction(const std::string& name);
    void Unload();
protected:
    ModHandle_t mHandle;
    std::string mTempFile;
};