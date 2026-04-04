#include "loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdexcept>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif

#if defined(__linux__)
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>
#include <iostream>
#include <fstream>
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

#elif defined(__APPLE__) || defined(__linux__)
    char path_template[] = "/tmp/mod_lib_XXXXXX";

    int fd = mkstemp(path_template);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temporary file: " + std::to_string(errno));
    }

#ifdef __linux__
    fchmod(fd, 0755);
#endif

    // mkstemp creates a physical file on the hard drive.
    // We can safely close the FD now; the file will remain on disk for you to open later.
    close(fd);
    return std::string(path_template);

#else
#error "Unsupported Operating System"
#endif
}

#ifdef __linux__
void FixELFHeader(const std::string& path) {
    // 1. Read the file into a buffer
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return;

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!in.read(reinterpret_cast<char*>(buffer.data()), size))
        return;
    in.close();

    if (buffer.size() < sizeof(Elf64_Ehdr))
        return;

    // 2. ELF Surgery
    Elf64_Ehdr* header = reinterpret_cast<Elf64_Ehdr*>(buffer.data());

    if (header->e_ident[EI_CLASS] == ELFCLASS64) {
        Elf64_Phdr* phdr = reinterpret_cast<Elf64_Phdr*>(buffer.data() + header->e_phoff);

        bool patched = false;
        for (int i = 0; i < header->e_phnum; i++) {
            // Target PT_NOTE (0x4) OR PT_GNU_EH_FRAME (0x6474e550)
            // TCC doesn't give us a NOTE, so we hijack the EH_FRAME header.
            if (phdr[i].p_type == PT_NOTE || phdr[i].p_type == 0x6474e550) {
                phdr[i].p_type = PT_GNU_STACK; // Change to 0x6474e551
                phdr[i].p_flags = PF_R | PF_W; // Set to RW (No Execute)
                phdr[i].p_align = 0x10;
                patched = true;
                break;
            }
        }

        if (!patched) {
            // Last resort: if we can't find either, we'll try to find any
            // non-critical segment that isn't LOAD (1) or DYNAMIC (2).
            for (int i = 0; i < header->e_phnum; i++) {
                if (phdr[i].p_type != PT_LOAD && phdr[i].p_type != PT_DYNAMIC) {
                    phdr[i].p_type = PT_GNU_STACK;
                    phdr[i].p_flags = PF_R | PF_W;
                    break;
                }
            }
        }
    }

    // 3. Write it back
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return;
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    out.close();

    // 4. Ensure permissions are set to allow execution of the file
    chmod(path.c_str(), 0755);
}
#endif

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
#elif defined(__linux__) || defined(__APPLE__)
#ifdef __linux__
    FixELFHeader(path);
#endif
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