# Building Ghostship

## Setting Up CubeOS

1. Clone the CubeOS repository and follow the build instructions.
2. Run the executable passing the path to your ROM as an argument.
3. You should now have a smcube.otr in the root folder of the repository, which will be needed when running this application.

## Windows

Requires:
  * Visual Studio 2022 Community Edition with the C++ feature set
  * One of the Windows SDKs that comes with Visual Studio, for example the current Windows 10 version 10.0.19041.0
  * The `MSVC v142 - VS 2019 C++ build tools` component of Visual Studio
  * Git (can be installed manually or as part of Visual Studio)
  * Cmake (can be installed via chocolatey or manually)

### Clone the Ghostship repository

_Note: Be sure to either clone with the ``--recursive`` flag or do ``git submodule update --init`` after cloning to pull in the libultraship submodule!_

### Building Ghostship

_Note: Instructions assume using powershell_
```powershell
# Navigate to the Ghostship repo within powershell. ie: cd "C:\yourpath\Ghostship"
cd Ghostship

# Setup cmake project
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 17 2022" -T v142 -A x64 -DCMAKE_BUILD_TYPE=Debug
# or for VS2019
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 16 2019" -T v142 -A x64 -DCMAKE_BUILD_TYPE=Debug
# Compile project
& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64 # --config Release (if you're packaging)

# If you need to clean the project you can run
& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64 --target clean

#### Visual Studio
To develop using Visual Studio you only need to use cmake to generate the solution file:
```powershell
# Generates Ghostship.sln at `build/x64` for Visual Studio 2022
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 17 2022" -T v142 -A x64
# or for Visual Studio 2019
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 16 2019" -T v142 -A x64
```

### Running the executable

1. Copy over the smcube.otr you have generated into the directory containing your executable, e.g. `.\build\x64\Debug`
2. From the GitHub, download the soh.otr and place that in the same directory as you smcube.otr
3. Currently, in order to run this on Windows, you must be using OpenGL instead of DirectX11. Change the `GetWindowBackend()` function in `libultraship\src\window\Window.cpp` to `return WindowBackend::SDL_OPENGL;` before building.
4. 

## Linux
Requires `gcc >= 10, x11, sdl2 >= 2.0.22, libpng, ninja, cmake, lld`

**Important: For maximum performance make sure you have ninja build tools installed!**

_Note: If you're using Visual Studio Code, the [cpack plugin](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) makes it very easy to just press run and debug._

### Building Ghostship

```bash
# Clone the repo
git clone https://github.com/HarbourMasters/Ghostship.git
cd Ghostship
# Clone the submodule libultraship
git submodule update --init
# Generate Ninja project
cmake -H. -Bbuild-cmake -GNinja -DCMAKE_BUILD_TYPE= Debug
# Compile the project
cmake --build build-cmake

# If you need to clean the project you can run
cmake --build build-cmake --target clean
```

### Running the executable

1. Copy over the smcube.otr you have generated into the root directory if the repository
2. From the GitHub, download the soh.otr and place that in the build directory, e.g. `./build-cmake`

## macOS
Requires Xcode (or xcode-tools) && `ninja, cmake` (can be installed via homebrew, macports, etc)

**Important: For maximum performance make sure you have ninja build tools installed!**

_Note: If you're using Visual Studio Code, the [cpack plugin](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) makes it very easy to just press run and debug._

```bash
# Clone the repo
git clone https://github.com/HarbourMasters/Ghostship.git
cd Ghostship
# Clone the submodule libultraship
git submodule update --init
# Generate Ninja project
cmake -H. -Bbuild-cmake -GNinja -DCMAKE_BUILD_TYPE=Debug
# Compile the project
cmake --build build-cmake

# If you need to clean the project you can run
cmake --build build-cmake --target clean
```

### Running the executable

1. Copy over the smcube.otr you have generated into the root directory if the repository
2. From the GitHub, download the soh.otr and place that in the build directory, e.g. `./build-cmake`
