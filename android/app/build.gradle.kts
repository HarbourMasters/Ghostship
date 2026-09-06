import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import java.security.MessageDigest
import java.util.Properties
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val repositoryRoot: File = rootProject.projectDir.parentFile

// One source of truth for the version: the CMake project declaration.
val projectVersion: String =
    Regex("""project\s*\([^)]*VERSION\s+([0-9.]+)""")
        .find(File(repositoryRoot, "CMakeLists.txt").readText())
        ?.groupValues?.get(1)
        ?: "0.0.0"

// Release signing: android/key.properties locally, the environment on CI. The
// file holds passwords, so it is gitignored and never read into the build
// output.
val keystoreProperties = Properties().apply {
    val file = rootProject.file("key.properties")
    if (file.isFile) file.inputStream().use { load(it) }
}

// With neither configured — a fork's CI, or a fresh clone — the release build
// falls back to the debug key so it still produces an installable APK rather
// than failing on a keystore nobody has.
val hasReleaseKeystore =
    keystoreProperties.getProperty("storeFile") != null || System.getenv("KEYSTORE_FILE") != null

// Torch runs on the device to turn the user's ROM into sm64.o2r, and it reads
// its extraction recipes off the filesystem. They ride into the APK in one zip
// alongside the engine archive; the launcher unpacks it into the app's external
// files directory on first run (see GameAssets.kt). The digest beside it is what
// the launcher compares against to decide whether to unpack again.
abstract class PackGameData : DefaultTask() {

    @get:InputDirectory
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val recipes: DirectoryProperty

    @get:InputFile
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val config: RegularFileProperty

    // A collection rather than an InputFile so that a missing archive reaches
    // the explanation below instead of Gradle's own input-validation failure.
    @get:InputFiles
    @get:PathSensitive(PathSensitivity.NONE)
    abstract val engineArchive: ConfigurableFileCollection

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun pack() {
        val engine = engineArchive.files.singleOrNull()?.takeIf(File::isFile)
            ?: throw GradleException(
                "ghostship.o2r not found at ${engineArchive.asPath} — build the GeneratePortO2R " +
                    "target on a desktop platform (or download the CI artifact) before building the APK."
            )

        val recipesRoot = recipes.get().asFile
        val entries = buildList {
            add("config.yml" to config.get().asFile)
            add(engine.name to engine)
            recipesRoot.walkTopDown().filter(File::isFile).forEach {
                add("assets/${it.relativeTo(recipesRoot).invariantSeparatorsPath}" to it)
            }
        }.sortedBy { it.first }

        val target = outputDir.get().asFile
        target.mkdirs()
        val zipFile = File(target, "gamedata.zip")

        // Sorted, at a fixed timestamp: a reproducible zip means a stable
        // digest, which means an app update only re-unpacks on the device when
        // the contents actually changed.
        ZipOutputStream(zipFile.outputStream().buffered()).use { zip ->
            entries.forEach { (name, file) ->
                zip.putNextEntry(ZipEntry(name).apply { time = FIXED_ENTRY_TIME })
                file.inputStream().use { it.copyTo(zip) }
                zip.closeEntry()
            }
        }

        val digest = MessageDigest.getInstance("MD5").digest(zipFile.readBytes())
        File(target, "gamedata.version").writeText(digest.joinToString("") { "%02x".format(it) })
    }

    private companion object {
        // 1980-02-01T00:00:00Z, the same instant Gradle's own archive tasks use
        // for this. Anything earlier risks falling before the 1980 floor that a
        // zip entry's DOS timestamp can represent.
        const val FIXED_ENTRY_TIME = 318211200000L
    }
}

// Wired through the variant API rather than added as a source-set srcDir. The
// srcDir form looks equivalent and is not: the asset merge takes the path but
// not the dependency on the task that fills it, so a clean checkout silently
// packages an APK whose assets are missing everything below — and the
// missing-archive check above never runs to say so.
androidComponents {
    onVariants { variant ->
        val packGameData =
            tasks.register<PackGameData>("pack${variant.name.replaceFirstChar(Char::titlecase)}GameData") {
                description = "Bundles the Torch recipes and the engine archive into the APK."
                recipes.set(File(repositoryRoot, "assets"))
                config.set(File(repositoryRoot, "config.yml"))
                engineArchive.from(File(repositoryRoot, "ghostship.o2r"))
            }
        variant.sources.assets?.addGeneratedSourceDirectory(packGameData, PackGameData::outputDir)
    }
}

android {
    namespace = "dev.net64.ghostship"
    compileSdk = 36
    ndkVersion = "30.0.15729638"

    defaultConfig {
        applicationId = "dev.net64.ghostship"
        minSdk = 24
        targetSdk = 36
        versionCode = 2
        versionName = projectVersion

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DUSE_OPENGLES=ON",
                    "-DSDL_SHARED=ON",
                    "-DSDL_STATIC=OFF",
                    "-DHAVE_LD_VERSION_SCRIPT=OFF",
                    // Forced for every variant, debug included. The root
                    // CMakeLists only tunes the Release flags (-O1 with
                    // -fno-strict-aliasing), and the game's C code relies on
                    // that: at -O2 and above, strict aliasing miscompiles it
                    // into graphical glitches. Left alone, the Android plugin
                    // builds the release variant as RelWithDebInfo (-O2) and
                    // walks straight into them.
                    "-DCMAKE_BUILD_TYPE=Release"
                )
                targets += "Ghostship"
            }
        }
    }

    signingConfigs {
        if (hasReleaseKeystore) {
            create("release") {
                val configuredStore = keystoreProperties.getProperty("storeFile")
                if (configuredStore != null) {
                    // A relative path in key.properties reads against the file's
                    // own directory, which is what someone editing it would
                    // expect.
                    storeFile = rootProject.file(configuredStore)
                    storePassword = keystoreProperties.getProperty("storePassword")
                    keyAlias = keystoreProperties.getProperty("keyAlias")
                    keyPassword = keystoreProperties.getProperty("keyPassword")
                } else {
                    storeFile = file(System.getenv("KEYSTORE_FILE"))
                    storePassword = System.getenv("KEYSTORE_PASSWORD")
                    keyAlias = System.getenv("KEY_ALIAS")
                    keyPassword = System.getenv("KEY_PASSWORD")
                }
            }
        }
    }

    buildTypes {
        debug {
            // No isJniDebuggable: the native side is built Release regardless
            // (see the CMAKE_BUILD_TYPE argument above), so there would be
            // nothing useful to attach to.
            applicationIdSuffix = ".debug"
        }
        release {
            // The APK is almost entirely native code and assets, so shrinking
            // the tiny Kotlin layer buys nothing and only risks stripping the
            // classes the JNI bridge looks up by name.
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName(if (hasReleaseKeystore) "release" else "debug")
            ndk {
                debugSymbolLevel = "SYMBOL_TABLE"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = File(repositoryRoot, "CMakeLists.txt")
            // libultraship needs CMake >= 3.24; the NDK-bundled 3.22.1 is too old.
            version = "3.30.3+"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    lint {
        abortOnError = false
    }
}

kotlin {
    compilerOptions {
        jvmTarget.set(JvmTarget.JVM_17)
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.activity:activity-ktx:1.9.3")
}
