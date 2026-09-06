/**
 * JNI surface for the Android app — the only two places the Kotlin side reaches
 * into native code:
 *
 *  - whether the engine's menu is up, which decides when the Mods button shows, and
 *  - game asset generation, which runs Torch against the user's ROM.
 *
 * On-screen controls are not here: the engine draws and handles its own (see
 * src/port/ui/TouchControls.cpp).
 *
 * The asset generation entry point is deliberately independent of libultraship:
 * it is called from the launcher activity, before SDL exists, so it cannot use
 * Ship::Context to discover paths. The launcher passes them in explicitly.
 */
#ifdef __ANDROID__

#include <jni.h>
#include <android/log.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <string>

#include "port/GameExtractor.h"
#include "port/ShipCompat.h"
#include "ship/Context.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace {

constexpr const char* kLogTag = "Ghostship";

std::string ToStdString(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string result = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(value, chars);
    }
    return result;
}

} // namespace

extern "C" {

/**
 * Whether libultraship's menu is currently up.
 *
 * The Mods button needs this because the menu is not only opened by the engine's
 * own on-screen toggle — a keyboard, a gamepad, or the menu closing itself all
 * change it behind Kotlin's back. Reading the engine's state instead of
 * mirroring it is what keeps the two from drifting apart.
 *
 * Called from the UI thread while the game thread renders. It reads a bool the
 * game thread may be writing, which is worth accepting here: the value only
 * decides whether one button is shown, and a torn read self-corrects on the
 * next poll.
 */
JNIEXPORT jboolean JNICALL Java_dev_net64_ghostship_MainActivity_isMenuOpen(JNIEnv*, jobject) {
    auto window = ShipCompat::GetWindow();
    if (window == nullptr) {
        return JNI_FALSE;
    }

    auto gui = window->GetGui();
    if (gui == nullptr) {
        return JNI_FALSE;
    }

    return gui->GetMenuOrMenubarVisible() ? JNI_TRUE : JNI_FALSE;
}

/**
 * Runs Torch over the ROM at romPath and writes sm64.o2r into destDir.
 *
 * sourceDir is the directory holding config.yml and assets/ymls (unpacked from
 * the APK by the launcher). Returns null on success, or a message describing
 * what went wrong.
 */
JNIEXPORT jstring JNICALL Java_dev_net64_ghostship_GameAssets_nativeGenerateGameArchive(JNIEnv* env, jobject,
                                                                                        jstring jRomPath,
                                                                                        jstring jSourceDir,
                                                                                        jstring jDestDir) {
    const std::string romPath = ToStdString(env, jRomPath);
    const std::string sourceDir = ToStdString(env, jSourceDir);
    const std::string destDir = ToStdString(env, jDestDir);

    const auto fail = [env](const std::string& message) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Asset generation failed: %s", message.c_str());
        return env->NewStringUTF(message.c_str());
    };

    GameExtractor extractor;
    if (!extractor.RunStandalone(romPath)) {
        return fail("Could not read a supported Super Mario 64 ROM at " + romPath + ".");
    }

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Extracting %s into %s", romPath.c_str(), destDir.c_str());

    std::string extractError;
    std::atomic<size_t> assetCount{ 0 };
    try {
        if (!extractor.GenerateOTRTo(assetCount, sourceDir, destDir)) {
            extractError = "Torch could not extract the ROM.";
        }
    } catch (const std::exception& error) {
        extractError = std::string("Torch could not extract the ROM: ") + error.what();
    } catch (...) { extractError = "Torch could not extract the ROM."; }

    // Companion is deliberately left alive, holding the ROM and every decoded
    // asset. Every other platform leaks it the same way, so ~Companion is dead
    // code that does not survive being run: it double-frees the parsed data and
    // dies in ~Painting on a garbage vptr. The launcher runs in its own process
    // (android:process=":launcher") and ends it once the archive is written, so
    // the OS reclaims all of this before the game starts.

    if (!extractError.empty()) {
        return fail(extractError);
    }

    // Init() logs and returns rather than throwing for a few failure modes
    // (missing config, unrecognised ROM), so confirm the archive really landed.
    std::error_code error;
    const std::filesystem::path archive = std::filesystem::path(destDir) / "sm64.o2r";
    if (!std::filesystem::exists(archive, error) || std::filesystem::file_size(archive, error) == 0) {
        return fail("Torch finished without producing sm64.o2r. Check that the ROM is Super Mario 64 (US or JP).");
    }

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Wrote %s", archive.c_str());
    return nullptr;
}

} // extern "C"

#endif // __ANDROID__
