#ifdef __ANDROID__

#include "AndroidUtils.h"

#include <SDL2/SDL.h>
#include <zip.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// SDL routes relative paths through the APK's AAssetManager on Android
static bool ReadApkAsset(const char* name, std::vector<uint8_t>& out) {
    SDL_RWops* rw = SDL_RWFromFile(name, "rb");
    if (rw == nullptr) {
        return false;
    }
    Sint64 size = SDL_RWsize(rw);
    if (size < 0) {
        SDL_RWclose(rw);
        return false;
    }
    out.resize((size_t)size);
    size_t read = SDL_RWread(rw, out.data(), 1, (size_t)size);
    SDL_RWclose(rw);
    return read == (size_t)size;
}

static std::string ReadTextFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return "";
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return text;
}

static bool ExtractZipBuffer(const std::vector<uint8_t>& buffer, const fs::path& destRoot) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t* source = zip_source_buffer_create(buffer.data(), buffer.size(), 0, &error);
    if (source == nullptr) {
        SDL_Log("gamedata: zip_source_buffer_create failed: %s", zip_error_strerror(&error));
        zip_error_fini(&error);
        return false;
    }

    zip_t* archive = zip_open_from_source(source, ZIP_RDONLY, &error);
    if (archive == nullptr) {
        SDL_Log("gamedata: zip_open_from_source failed: %s", zip_error_strerror(&error));
        zip_source_free(source);
        zip_error_fini(&error);
        return false;
    }

    bool ok = true;
    zip_int64_t numEntries = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < numEntries && ok; i++) {
        const char* name = zip_get_name(archive, i, 0);
        if (name == nullptr || name[0] == '\0') {
            continue;
        }
        std::string entry(name);
        if (entry.find("..") != std::string::npos || entry[0] == '/') {
            SDL_Log("gamedata: skipping suspicious entry %s", name);
            continue;
        }

        fs::path target = destRoot / entry;
        std::error_code ec;
        if (entry.back() == '/') {
            fs::create_directories(target, ec);
            continue;
        }
        fs::create_directories(target.parent_path(), ec);

        zip_file_t* zf = zip_fopen_index(archive, i, 0);
        if (zf == nullptr) {
            SDL_Log("gamedata: zip_fopen_index(%s) failed: %s", name, zip_strerror(archive));
            ok = false;
            break;
        }

        FILE* outFile = fopen(target.c_str(), "wb");
        if (outFile == nullptr) {
            SDL_Log("gamedata: cannot write %s", target.c_str());
            zip_fclose(zf);
            ok = false;
            break;
        }

        char chunk[64 * 1024];
        zip_int64_t n;
        while ((n = zip_fread(zf, chunk, sizeof(chunk))) > 0) {
            if (fwrite(chunk, 1, (size_t)n, outFile) != (size_t)n) {
                SDL_Log("gamedata: short write on %s", target.c_str());
                ok = false;
                break;
            }
        }
        if (n < 0) {
            SDL_Log("gamedata: zip_fread(%s) failed", name);
            ok = false;
        }
        fclose(outFile);
        zip_fclose(zf);
    }

    zip_close(archive); // also frees the source
    zip_error_fini(&error);
    return ok;
}

bool Android_SyncPackagedData() {
    const char* storage = SDL_AndroidGetExternalStoragePath();
    if (storage == nullptr) {
        storage = SDL_AndroidGetInternalStoragePath();
    }
    if (storage == nullptr) {
        SDL_Log("gamedata: no storage path available");
        return false;
    }
    fs::path destRoot(storage);

    std::vector<uint8_t> versionBuf;
    std::string packagedVersion;
    if (ReadApkAsset("gamedata.version", versionBuf)) {
        packagedVersion.assign(versionBuf.begin(), versionBuf.end());
    }

    fs::path stampPath = destRoot / ".gamedata.version";
    if (!packagedVersion.empty() && ReadTextFile(stampPath) == packagedVersion) {
        return true;
    }

    std::vector<uint8_t> zipBuf;
    if (!ReadApkAsset("gamedata.zip", zipBuf)) {
        SDL_Log("gamedata: gamedata.zip missing from APK assets");
        return false;
    }

    SDL_Log("gamedata: extracting packaged data to %s", destRoot.c_str());
    if (!ExtractZipBuffer(zipBuf, destRoot)) {
        return false;
    }

    if (!packagedVersion.empty()) {
        std::ofstream stamp(stampPath, std::ios::binary | std::ios::trunc);
        stamp << packagedVersion;
    }
    return true;
}

#endif
