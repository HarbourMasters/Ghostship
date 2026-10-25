# Building Ghostship

## Windows

Requires:
  * At least 8GB of RAM (machines with 4GB have seen complier failures)
  * Visual Studio 2022 Community Edition with the C++ feature set
  * One of the Windows SDKs that comes with Visual Studio, for example the current Windows 10 version 10.0.19041.0
  * The `MSVC v143 - VS 2022 C++ build tools` component of Visual Studio
  * Python 3 (can be installed manually or as part of Visual Studio)
  * Git (can be installed manually or as part of Visual Studio)
  * Cmake (can be installed via chocolatey or manually)

During installation, check the "Desktop development with C++" feature set:

![image](https://user-images.githubusercontent.com/30329717/183511274-d11aceea-7900-46ec-acb6-3f2cc110021a.png)
Doing so should also check one of the Windows SDKs by default.  Then, in the installation details in the right-hand column, make sure you also check the v143 toolset. This is often done by default.

It is recommended that you install Python and Git standalone, the install process in VS Installer has given some issues in the past.

1. Clone the Ghostship repository

_Note: Be sure to either clone with the ``--recursive`` flag or do ``git submodule update --init`` after cloning to pull in the libultraship submodule!_

2. After setup and initial build, use the built-in OTR extraction to make your sm64.o2r file.

_Note: Instructions assume using powershell_
```powershell
# Navigate to the ghostship repo within powershell. ie: cd "C:\yourpath\ghostship"
cd ghostship

# Setup cmake project
# Add `-DCMAKE_BUILD_TYPE:STRING=Release` if you're packaging
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 17 2022" -T v143 -A x64

# Generate sm64.o2r
& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64 --target ExtractAssets

# Generate ghostship.o2r
& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64 --target GeneratePortO2R

# Compile project
# Add `--config Release` if you're packaging
& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64

# Now you can run the executable in .\build\x64 or run in Visual Studio
```

### Developing Ghostship
With the cmake build system you have two options for working on the project:

#### Visual Studio
To develop using Visual Studio you only need to use cmake to generate the solution file:
```powershell
# Generates ghostship.sln at `build/x64` for Visual Studio 2022
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 17 2022" -T v143 -A x64
```

#### Visual Studio Code or another editor
To develop using Visual Studio Code or another editor you only need to open the repository in it.
To build you'll need to follow the instructions from the building section.

_Note: If you're using Visual Studio Code, the [cpack plugin](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) makes it very easy to just press run and debug._

_Experimental: You can also use another build system entirely rather than MSVC like [Ninja](https://ninja-build.org/) for possibly better performance._


### Generating the distributable
After compiling the project you can generate the distributable by running:
```powershell
# Go to build folder
cd "build/x64"
# Generate
& 'C:\Program Files\CMake\bin\cpack.exe' -G ZIP
```

### Additional CMake Targets
#### Clean
```powershell
# If you need to clean the project you can run
C:\Program Files\CMake\bin\cmake.exe --build build-cmake --target clean
```

## Xbox (Developer Mode)

Builds an x64 UWP app package for a retail Xbox in [Developer Mode](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/devkit-activation).

Needs Visual Studio 2022 with the **Universal Windows Platform development** workload and its C++ component, which are separate from the desktop C++ workload:

```sh
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vs_installer.exe" modify ^
  --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" ^
  --add Microsoft.VisualStudio.Workload.Universal ^
  --add Microsoft.VisualStudio.ComponentGroup.UWP.VC --passive --norestart
```

The package has to be signed with a certificate whose subject matches the manifest's `Publisher` (`CN=Lywx` by default). Create one once; it stays in your certificate store:

```powershell
$cert = New-SelfSignedCertificate -Type Custom -Subject "CN=Lywx" -KeyUsage DigitalSignature `
  -FriendlyName "Ghostship Xbox Dev Signing" -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") `
  -NotAfter (Get-Date).AddYears(3)
$cert.Thumbprint
```

Build with Ninja from a shell where `vcvars64.bat` has been run, so `WindowsApp.lib` and the app container CRT are found:

```sh
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build-uwp -G Ninja -DCMAKE_BUILD_TYPE=Release -DXBOX_UWP=ON ^
  -DUWP_PACKAGE_CERT_THUMBPRINT=<thumbprint from above>
cmake --build build-uwp --target ghostship_uwp_package
```

That produces a signed `build-uwp/Ghostship.msix`. Visual Studio's remote deploy is not available for this target: CMake links the executable with `/APPCONTAINER`, packs it with `MakeAppx` and signs it with `SignTool` itself.

Use `-DUWP_PACKAGE_CERT=<path to .pfx>` (plus `-DUWP_PACKAGE_CERT_PASSWORD=`) instead of a thumbprint where there is no certificate store, and `-DUWP_PACKAGE_PUBLISHER="CN=..."` for a certificate with a different subject.

The console has to trust the certificate. Export its public half and pass it on the first deploy, which uploads through the Device Portal and waits for the install:

```powershell
Export-Certificate -Cert "Cert:\CurrentUser\My\<thumbprint>" -FilePath ghostship-public.cer
tools\deploy-xbox.ps1 -Console <console ip> -Package build-uwp\Ghostship.msix -Certificate ghostship-public.cer
```

The console also needs the VCLibs framework package once, `Microsoft.VCLibs.x64.14.00.appx` from `Microsoft SDKs\Windows Kits\10\ExtensionSDKs\Microsoft.VCLibs`; without it the app fails to activate with `0x8027025B`.

Repeat deploys install over the top, which keeps the app's LocalState (the game archive, saves and mods). `-Clean` uninstalls first and **deletes** all of it.

The Xbox target renders with DirectX 11 only, and has no Satella connection, so online mod features are unavailable there. Mods and scripting work as on the desktop.

## Linux
### Install dependencies
#### Debian/Ubuntu
```sh
# using gcc
apt-get install gcc g++ git cmake ninja-build lsb-release libsdl2-dev libpng-dev libsdl2-net-dev libzip-dev zipcmp zipmerge ziptool nlohmann-json3-dev libtinyxml2-dev libspdlog-dev libboost-dev libopengl-dev libogg-dev libvorbis-dev libmbedtls-dev

# or using clang
apt-get install clang git cmake ninja-build lsb-release libsdl2-dev libpng-dev libsdl2-net-dev libzip-dev zipcmp zipmerge ziptool nlohmann-json3-dev libtinyxml2-dev libspdlog-dev libboost-dev libopengl-dev libogg-dev libvorbis-dev libmbedtls-dev
```
#### Arch
```sh
# using gcc
pacman -S gcc git cmake ninja make lsb-release sdl2-compat libpng libzip nlohmann-json tinyxml2 spdlog libogg libvorbis

# or using clang
pacman -S clang git cmake ninja make lsb-release sdl2-compat libpng libzip nlohmann-json tinyxml2 spdlog libogg libvorbis

# optional, for the Vulkan renderer (you also need the Vulkan driver for your GPU)
pacman -S vulkan-headers vulkan-icd-loader shaderc
```
_Note: Arch's `mbedtls` package is version 4.x, which moved the `ctr_drbg.h` and `entropy.h` headers used by the Satella networking code (`src/port/net/PlayerIdentity.cpp`) to `mbedtls/private/`. Unless you provide mbedTLS 3.x yourself, configure with `-DUSE_SATELLA=OFF`._

#### Fedora
```sh
# using gcc
dnf install gcc gcc-c++ git cmake ninja-build lsb_release SDL2-devel libpng-devel libzip-devel libzip-tools nlohmann-json-devel tinyxml2-devel spdlog-devel boost-devel libogg-devel libvorbis-devel mbedtls-devel

# or using clang
dnf install clang git cmake ninja-build lsb_release SDL2-devel libpng-devel libzip-devel libzip-tools nlohmann-json-devel tinyxml2-devel spdlog-devel boost-devel libogg-devel libvorbis-devel mbedtls-devel
```
#### openSUSE
```sh
# using gcc
zypper in gcc gcc-c++ git cmake ninja SDL2-devel libpng16-devel libzip-devel libzip-tools nlohmann_json-devel tinyxml2-devel spdlog-devel libogg-devel libvorbis-devel libmbedtls-devel

# or using clang
zypper in clang libstdc++-devel git cmake ninja SDL2-devel libpng16-devel libzip-devel libzip-tools nlohmann_json-devel tinyxml2-devel spdlog-devel libogg-devel libvorbis-devel libmbedtls-devel
```

### Build

_Note: If you're using Visual Studio Code, the [CMake Tools plugin](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) makes it very easy to just press run and debug._

```bash
# Clone the repo and enter the directory
git clone https://github.com/HarbourMasters/ghostship.git
cd ghostship

# Clone the submodules
git submodule update --init

# Generate Ninja project
# Add `-DCMAKE_BUILD_TYPE:STRING=Release` if you're packaging
# Add `-DPython3_EXECUTABLE=$(which python3)` if you are using non-standard Python installations such as PyEnv
# Add `-DUSE_SATELLA=OFF` if your distribution only ships mbedTLS 4.x (see the Arch note above)
cmake -H. -Bbuild-cmake -GNinja

# Generate sm64.o2r
cmake --build build-cmake --target ExtractAssets

# Generate ghostship.o2r
cmake --build build-cmake --target GeneratePortO2R

# Compile the project
cmake --build build-cmake

# Now you can run the executable file:
./build-cmake/Ghostship
# To develop the project open the repository in VSCode (or your preferred editor)
```

### Generate a distributable
After compiling the project you can generate a distributable by running of the following:
```bash
# Go to build folder
cd build-cmake
# Generate
cpack -G DEB
cpack -G ZIP
cpack -G External (creates appimage)
```

### Additional CMake Targets
#### Clean
```bash
# If you need to clean the project you can run
cmake --build build-cmake --target clean
```

## macOS
Requires Xcode (or xcode-tools) && `sdl2, libpng, glew, ninja, cmake, nlohmann-json, tinyxml2, libzip, vorbis-tools, mbedtls` (can be installed via homebrew, macports, etc)

**Important: For maximum performance make sure you have ninja build tools installed!**

_Note: If you're using Visual Studio Code, the [cpack plugin](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) makes it very easy to just press run and debug._

```bash
# Clone the repo
git clone https://github.com/HarbourMasters/ghostship.git
cd ghostship
# Clone the submodule libultraship
git submodule update --init

# Generate Ninja project
# Add `-DCMAKE_BUILD_TYPE:STRING=Release` if you're packaging
cmake -H. -Bbuild-cmake -GNinja

# Generate sm64.o2r
cmake --build build-cmake --target ExtractAssets

# Generate ghostship.o2r
cmake --build build-cmake --target GeneratePortO2R

# Compile the project
# Add `--config Release` if you're packaging
cmake --build build-cmake

# Now you can run the executable file:
./build-cmake/Ghostship
# To develop the project open the repository in VSCode (or your preferred editor)
```

### Generating a distributable
After compiling the project you can generate a distributable by running of the following:
```bash
# Go to build folder
cd build-cmake
# Generate
cpack
```

### Additional CMake Targets
#### Clean
```bash
# If you need to clean the project you can run
cmake --build build-cmake --target clean
```

# Compatible Roms
See [`supportedHashes.json`](supportedHashes.json)

## Getting CI to work on your fork

The CI works via [Github Actions](https://github.com/features/actions) where we mostly make use of machines hosted by Github; except for the very first step of the CI process called "Extract assets". This steps extracts assets from the game file and generates an "assets" folder in `mm/`.

To get this step working on your fork, you'll need to add a machine to your own repository as a self-hosted runner via "Settings > Actions > Runners" in your repository settings. Make sure to add the 'asset-builder' tag to your newly added runner to assign it to run this step. To setup your runner as a service read the docs [here](https://docs.github.com/en/actions/hosting-your-own-runners/configuring-the-self-hosted-runner-application-as-a-service?platform=linux).

### Runner on Windows
You'll have to enable the ability to run unsigned scripts through PowerShell. To do this, open Powershell as administrator and run `set-executionpolicy remotesigned`. Most dependencies get installed as part of the CI process. You will also need to separately install 7z and add it to the PATH so `7z` can be run as a command. [Chocolatey](https://chocolatey.org/) or other package managers can be used to install it easily.

### Runner on UNIX systems
If you're on macOS or Linux take a look at `macports-deps.txt` or `apt-deps.txt` to see the dependencies expected to be on your machine.
