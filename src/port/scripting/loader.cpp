#include "loader.h"

#if defined(__linux__)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#if defined(__linux__)
#include <sys/mman.h>
#endif

#include <string>
#include <stdexcept>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif

#if defined(__linux__)
#include <sys/mman.h>
#endif

std::string ModInstance::GenerateTempFile() {
#if defined(_WIN32) || defined(__CYGWIN__)
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];

    if (!GetTempPathA(MAX_PATH, tempPath)) {
        throw std::runtime_error("Failed to get temp path: " + std::to_string(GetLastError()));
    }

    // GetTempFileNameA automatically creates an empty file on disk for us.
    if (!GetTempFileNameA(tempPath, "mod", 0, tempFileName)) {
        throw std::runtime_error("Failed to create temp file: " + std::to_string(GetLastError()));
    }

    return std::string(tempFileName);

#elif defined(__linux__)
    int fd = memfd_create("virtual_mod_lib", 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create memfd: " + std::to_string(errno));
    }

    // IMPORTANT: We deliberately DO NOT close(fd) here!
    // A memfd only exists in RAM as long as an open file descriptor points to it.
    // If we closed it, the file would instantly vanish and the path would become invalid.
    char path[256];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    return std::string(path);

#elif defined(__APPLE__)
    char path_template[] = "/tmp/mod_lib_XXXXXX";

    int fd = mkstemp(path_template);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temporary file: " + std::to_string(errno));
    }

    // mkstemp creates a physical file on the hard drive.
    // We can safely close the FD now; the file will remain on disk for you to open later.
    close(fd);
    return std::string(path_template);

#else
#error "Unsupported Operating System"
#endif
}

void ModInstance::Init(const std::string& path) {
#if defined(_WIN32) || defined(__CYGWIN__)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (handle) {
        this->mHandle = handle;
        this->mTempFile = path;
        return;
    } else {
        throw std::runtime_error("Failed to load library: " + std::to_string(GetLastError()));
    }
#elif defined(__linux__)
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (handle) {
        this->mHandle = handle;
        return;
    } else {
        throw std::runtime_error("Failed to load library: " + std::string(dlerror()));
    }
#elif defined(__APPLE__)
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (handle) {
        this->mHandle = handle;
        this->mTempFile = path;
    } else {
        unlink(path.c_str());
        throw std::runtime_error("Failed to load library: " + std::string(dlerror()));
    }

    unlink(path.c_str());
#else
#error "Unsupported Operating System"
#endif
}

ModFunc_t ModInstance::GetFunction(const std::string& name) {
    if (!mHandle)
        return nullptr;
#if defined(_WIN32) || defined(__CYGWIN__)
    return (ModFunc_t)GetProcAddress((HMODULE)mHandle, name.c_str());
#else
    return (ModFunc_t)dlsym(mHandle, name.c_str());
#endif
}

void ModInstance::Unload() {
    if (!mHandle) {
        return;
    }

#if defined(_WIN32) || defined(__CYGWIN__)
    FreeLibrary((HMODULE)mHandle);
    DeleteFileA(mTempFile.c_str());
#endif
    mHandle = nullptr;
}