import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import java.security.MessageDigest
import java.util.Properties

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
// files directory on first run (see GameAssets.kt).
val gamedataDir = layout.buildDirectory.dir("generated/gamedata")

val packGameData = tasks.register<Zip>("packGameData") {
    description = "Bundles the Torch recipes and the engine archive into the APK."
    val engineArchive = File(repositoryRoot, "ghostship.o2r")
    doFirst {
        if (!engineArchive.exists()) {
            throw GradleException(
                "ghostship.o2r not found at $engineArchive — build the GeneratePortO2R target " +
                    "on a desktop platform (or download the CI artifact) before building the APK."
            )
        }
    }
    from(File(repositoryRoot, "assets")) { into("assets") }
    from(File(repositoryRoot, "config.yml"))
    from(engineArchive)
    archiveFileName.set("gamedata.zip")
    destinationDirectory.set(gamedataDir)
    // A reproducible zip means a stable digest, which means an app update only
    // re-unpacks when the contents actually changed.
    isPreserveFileTimestamps = false
    isReproducibleFileOrder = true
}

val stampGameData = tasks.register("stampGameData") {
    description = "Writes the digest the launcher compares against to decide whether to unpack."
    dependsOn(packGameData)
    val stampFile = gamedataDir.get().file("gamedata.version").asFile
    outputs.file(stampFile)
    doLast {
        val zipFile = gamedataDir.get().file("gamedata.zip").asFile
        val digest = MessageDigest.getInstance("MD5").digest(zipFile.readBytes())
        stampFile.writeText(digest.joinToString("") { "%02x".format(it) })
    }
}

// Derived from the task rather than named as a plain path, so every consumer
// picks up the dependency. Naming the directory directly only looks fine until
// something other than the asset merge reads it — release builds run lint-vital
// over the source sets, and that fails on the missing dependency.
val stagedGameData: Provider<File> = stampGameData.map { gamedataDir.get().asFile }

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

    sourceSets.named("main") {
        assets.srcDir(stagedGameData)
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
