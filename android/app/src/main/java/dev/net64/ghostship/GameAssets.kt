package dev.net64.ghostship

import android.content.Context
import android.util.Log
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.security.MessageDigest
import java.util.zip.ZipInputStream

/**
 * Everything the game needs on disk, and how it gets there.
 *
 * libultraship resolves every runtime path on Android through
 * `SDL_AndroidGetExternalStoragePath()`, i.e. `getExternalFilesDir(null)`. That
 * is also somewhere the user can reach with a file manager, which is what makes
 * dropping mods in possible, so everything lives there:
 *
 *     Android/data/dev.net64.ghostship/files/
 *       baserom.us.z64       the user's ROM, copied in by the launcher
 *       config.yml, assets/  Torch's extraction recipes, unpacked from the APK
 *       ghostship.o2r        engine assets, built on CI and shipped in the APK
 *       sm64.o2r             generated on-device from the ROM
 *       mods/                user mods (.o2r, .zip or plain folders)
 *
 * The unpacking happens here rather than inside the engine because Torch needs
 * config.yml and assets/ on disk before the game process starts, not after.
 * The zip itself is assembled by android/app/build.gradle.kts.
 */
object GameAssets {

    // Named for the engine's own mobile fallback (GameExtractor.cpp), so a
    // re-extraction triggered from inside the game finds it too. Torch hashes
    // the contents, so a JP ROM under this name still extracts correctly.
    const val ROM_NAME = "baserom.us.z64"

    /** The ROMs Torch knows how to extract, by SHA-1. */
    val SUPPORTED_ROMS = mapOf(
        "9bef1128717f958171a4afac3ed78ee2bb4e86ce" to "Super Mario 64 (US)",
        "8a20a5c83d6ceb0f0506cfc9fa20d8f438cafe51" to "Super Mario 64 (JP)"
    )

    private const val TAG = "GameAssets"
    private const val GAME_ARCHIVE = "sm64.o2r"
    private const val CONTROLLER_DB = "gamecontrollerdb.txt"
    private const val TORCH_HASHES = "torch.hash.yml"
    private const val GAMEDATA_ZIP = "gamedata.zip"
    private const val GAMEDATA_VERSION = "gamedata.version"
    private const val STAMP = ".gamedata.version"

    fun gameDir(context: Context): File =
        (context.getExternalFilesDir(null) ?: context.filesDir).also { it.mkdirs() }

    fun romFile(context: Context) = File(gameDir(context), ROM_NAME)

    fun gameArchive(context: Context) = File(gameDir(context), GAME_ARCHIVE)

    fun modsDir(context: Context) = File(gameDir(context), "mods")

    fun isExtracted(context: Context) = gameArchive(context).length() > 0

    /**
     * Unpacks the APK-bundled extraction inputs into [gameDir].
     *
     * Re-runs whenever the packaged zip changes, so an app update ships new
     * recipes and a new ghostship.o2r without the user clearing data. Returns an
     * error message, or null when the directory is ready.
     */
    fun stageBundledAssets(context: Context): String? {
        val target = gameDir(context)
        val stamp = File(target, STAMP)

        return try {
            val expected = context.assets.open(GAMEDATA_VERSION).use { it.readBytes().decodeToString() }
            if (stamp.takeIf { it.isFile }?.readText() == expected) {
                return null
            }

            context.assets.open(GAMEDATA_ZIP).use { input ->
                ZipInputStream(input.buffered()).use { zip ->
                    while (true) {
                        val entry = zip.nextEntry ?: break
                        val destination = File(target, entry.name).canonicalFile

                        // An entry named ../../something would otherwise write
                        // outside the game directory.
                        if (!destination.path.startsWith(target.canonicalPath + File.separator)) {
                            Log.w(TAG, "Skipping suspicious zip entry ${entry.name}")
                            zip.closeEntry()
                            continue
                        }

                        if (entry.isDirectory) {
                            destination.mkdirs()
                        } else {
                            destination.parentFile?.mkdirs()
                            FileOutputStream(destination).use { output -> zip.copyTo(output, COPY_BUFFER_BYTES) }
                        }
                        zip.closeEntry()
                    }
                }
            }

            // Users are meant to edit this one, so only seed it once.
            val controllerDb = File(target, CONTROLLER_DB)
            if (!controllerDb.isFile) {
                copyAsset(context, CONTROLLER_DB, controllerDb)
            }

            val mods = modsDir(context)
            mods.mkdirs()
            if (mods.list().isNullOrEmpty()) {
                // Only a hint for the player, and nothing reads it back, so it
                // must not fail an extraction that has otherwise succeeded —
                // which is what a missing placeholder used to do.
                try {
                    copyAsset(context, "mods/place_mods_here.txt", File(mods, "place_mods_here.txt"))
                } catch (error: IOException) {
                    Log.w(TAG, "Could not write the mods placeholder", error)
                }
            }

            // New recipes mean the previous archive is stale, and Torch skips
            // work whose inputs it believes are unchanged.
            gameArchive(context).delete()
            File(target, TORCH_HASHES).delete()

            stamp.writeText(expected)
            null
        } catch (error: IOException) {
            Log.e(TAG, "Could not unpack the bundled assets", error)
            "Could not unpack the bundled game files: ${error.message}"
        }
    }

    /**
     * Runs Torch over [romFile] to produce sm64.o2r. Blocking and slow — expect
     * a minute or more on older hardware. Returns an error message, or null on
     * success.
     */
    fun generateGameArchive(context: Context): String? {
        val dir = gameDir(context)
        return nativeGenerateGameArchive(romFile(context).absolutePath, dir.absolutePath, dir.absolutePath)
    }

    fun sha1Of(file: File): String {
        val digest = MessageDigest.getInstance("SHA-1")
        file.inputStream().use { input ->
            val buffer = ByteArray(COPY_BUFFER_BYTES)
            while (true) {
                val read = input.read(buffer)
                if (read < 0) break
                digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private const val COPY_BUFFER_BYTES = 1 shl 17

    private fun copyAsset(context: Context, assetPath: String, target: File) {
        target.parentFile?.mkdirs()
        context.assets.open(assetPath).use { input ->
            FileOutputStream(target).use { output ->
                input.copyTo(output)
                output.fd.sync()
            }
        }
    }

    private external fun nativeGenerateGameArchive(rom: String, sourceDir: String, destDir: String): String?

    init {
        // libmain.so carries both the game and the Torch extractor.
        System.loadLibrary("SDL2")
        System.loadLibrary("main")
    }
}
