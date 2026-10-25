package dev.net64.ghostship

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import android.util.Log
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import java.util.zip.ZipOutputStream

/**
 * The game's save files.
 *
 * Ghostship keeps one JSON file per save slot in a `saves/` folder (see
 * src/port/data/Saves.cpp), so a save is `saves/save_N.json`. On Android that
 * folder is in `Android/media`, see [savesDir].
 * A `default.sav` left over from an older build is migrated into that folder on
 * the next launch, so it is deliberately not listed here.
 *
 * Files are matched by extension rather than by name, so a slot added upstream
 * is picked up without changes here.
 */
object SaveStore {

    private const val TAG = "SaveStore"
    private const val COPY_BUFFER = 1 shl 16

    const val BUNDLE_NAME = "ghostship-saves.zip"

    data class Save(val file: File) {
        val label: String
            get() = file.name.removeSuffix(".json")
                .removePrefix("save_")
                .toIntOrNull()
                ?.let { "File ${it + 1}" }
                ?: file.name
    }

    /**
     * Saves live in `Android/media/<package>/saves` rather than beside the archives in `Android/data`, because sync
     * apps like Syncthing can't reach `Android/data` from Android 11 on. The game is pointed here through
     * [SAVES_DIR_ENV] (see MainActivity and src/port/data/Saves.cpp).
     */
    @Suppress("DEPRECATION") // externalMediaDirs is the only API that returns Android/media.
    fun savesDir(context: Context): File {
        val media = context.externalMediaDirs.firstOrNull { it != null }
        val dir = if (media != null) File(media, "saves") else legacySavesDir(context)
        dir.mkdirs()
        migrateLegacySaves(context, dir)
        return dir
    }

    const val SAVES_DIR_ENV = "GHOSTSHIP_SAVES_DIR"

    private fun legacySavesDir(context: Context) = File(GameAssets.gameDir(context), "saves")

    /**
     * Copies saves from the old `Android/data/.../saves` folder once, without overwriting anything already in the new
     * one, then renames the old folder so it isn't picked up or mistaken for the live saves again.
     */
    private fun migrateLegacySaves(context: Context, target: File) {
        val legacy = legacySavesDir(context)
        if (legacy.absoluteFile == target.absoluteFile || !legacy.isDirectory) {
            return
        }
        try {
            legacy.listFiles()?.filter { it.isFile }?.forEach { source ->
                val destination = File(target, source.name)
                if (!destination.exists()) {
                    source.copyTo(destination)
                }
            }
            legacy.renameTo(File(legacy.parentFile, "saves-moved-to-media"))
            Log.i(TAG, "Moved saves to ${target.absolutePath}")
        } catch (e: Exception) {
            Log.e(TAG, "Could not move saves to ${target.absolutePath}", e)
        }
    }

    fun list(context: Context): List<Save> =
        (savesDir(context).listFiles() ?: emptyArray())
            .filter { it.isFile && it.name.endsWith(".json", ignoreCase = true) }
            .sortedBy { it.name }
            .map { Save(it) }

    fun delete(save: Save): String? =
        if (save.file.delete()) null else "Could not delete ${save.label}."

    fun export(context: Context, save: Save, destination: Uri): String? = runOrReport("export") {
        context.contentResolver.openOutputStream(destination).use { output ->
            requireNotNull(output) { "The document provider did not open the destination." }
            save.file.inputStream().use { input -> input.copyTo(output, COPY_BUFFER) }
        }
    }

    fun exportAll(context: Context, destination: Uri): String? = runOrReport("export") {
        val saves = list(context)
        check(saves.isNotEmpty()) { "There are no saves to export yet." }

        context.contentResolver.openOutputStream(destination).use { output ->
            requireNotNull(output) { "The document provider did not open the destination." }
            ZipOutputStream(output.buffered()).use { zip ->
                for (save in saves) {
                    zip.putNextEntry(ZipEntry(save.file.name))
                    save.file.inputStream().use { input -> input.copyTo(zip, COPY_BUFFER) }
                    zip.closeEntry()
                }
            }
        }
    }

    /**
     * Restores a single save, or every save out of a zip written by
     * [exportAll]. Existing files of the same name are replaced.
     */
    fun import(context: Context, uri: Uri): String? {
        val displayName = queryDisplayName(context, uri) ?: return "Could not read the file's name."

        return when {
            displayName.endsWith(".zip", true) -> importBundle(context, uri)
            displayName.endsWith(".json", true) -> runOrReport("import") {
                context.contentResolver.openInputStream(uri).use { input ->
                    requireNotNull(input) { "The document provider did not open $displayName." }
                    FileOutputStream(File(savesDir(context), displayName)).use { output ->
                        input.copyTo(output, COPY_BUFFER)
                    }
                }
            }
            else -> "$displayName is not a save. Saves are .json files, or a .zip of them."
        }
    }

    private fun importBundle(context: Context, uri: Uri): String? = runOrReport("import") {
        val target = savesDir(context)
        var restored = 0

        context.contentResolver.openInputStream(uri).use { input ->
            requireNotNull(input) { "The document provider did not open the archive." }
            ZipInputStream(input.buffered()).use { zip ->
                while (true) {
                    val entry = zip.nextEntry ?: break
                    val name = File(entry.name).name

                    // Only ever write a plain .json straight into the saves
                    // directory; an archive from elsewhere could otherwise
                    // carry entries like ../../ pointing out of it.
                    if (entry.isDirectory || !name.endsWith(".json", true)) {
                        zip.closeEntry()
                        continue
                    }

                    FileOutputStream(File(target, name)).use { output -> zip.copyTo(output, COPY_BUFFER) }
                    zip.closeEntry()
                    restored++
                }
            }
        }

        check(restored > 0) { "That archive has no save files in it." }
    }

    private fun runOrReport(verb: String, work: () -> Unit): String? = try {
        work()
        null
    } catch (error: Exception) {
        Log.e(TAG, "Save $verb failed", error)
        "Could not $verb: ${error.message ?: error}"
    }

    private fun queryDisplayName(context: Context, uri: Uri): String? =
        context.contentResolver.query(uri, arrayOf(DocumentsContract.Document.COLUMN_DISPLAY_NAME), null, null, null)
            ?.use { cursor -> if (cursor.moveToFirst()) cursor.getString(0) else null }
}
