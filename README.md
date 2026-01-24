# Ghostship

Lead Developers:
* [Lywx](https://www.github.com/kiritodv)

## Discord

Official Discord: https://discord.com/invite/shipofharkinian

If you're having any trouble after reading through this `README`, feel free ask for help in the Ghostship Support text channels. Please keep in mind that we do not condone piracy.

# Quick Start

Ghostship does not include any copyrighted assets.  You are required to provide a supported copy of the game.

### 1. Verify your ROM dump
The supported ROMs are US and JP versions. You can verify you have dumped a supported copy of the game by using the SHA-1 File Checksum Online at https://www.romhacking.net/hash/. 

* The SHA-1 hash for a US ROM is 9bef1128717f958171a4afac3ed78ee2bb4e86ce.
* The SHA-1 hash for a JP ROM is 8a20a5c83d6ceb0f0506cfc9fa20d8f438cafe51.

### 2. Verify your ROM is in .z64 format
Your ROM needs to be in .z64 format. If it's in .n64 format, use the following to convert it to a .z64: https://hack64.net/tools/swapper.php

### 2. Download Ghostship from [Releases](https://github.com/HarbourMasters/Ghostship/releases)

### 3. Generating the OTR from the ROM and Play!
#### Windows
* Extract every file from the zip into a folder of your choosing.
* Run Ghostship.exe and select your US or JP ROM.

#### Linux
* Extract every file from the zip into a folder of your choosing.
* Execute Ghostship.appimage. You may have to chmod +x the appimage via terminal.

#### MacOS
* Extract every file from the zip into a folder of your choosing.
* Run Ghostship and select your US or JP ROM.

#### Nintendo Switch
* Run one of the PC releases to generate an `sm64.o2r` file. After launching the game on PC, you will be able to find these files in the same directory as `Ghostship.exe` or `Ghostship.appimage`.
* Copy the files to your sd card

# Configuration

### Default keyboard configuration
| N64 | A | B | Z | Start | Analog stick | C buttons | D-Pad |
| - | - | - | - | - | - | - | - |
| Keyboard | X | C | Z | Space | WASD | Arrow keys | TFGH |

### Other shortcuts
| Keys | Action |
| - | - |
| Esc | Toggle menu |
| Ctrl+R | Reset (inside levels) |
| F11 | Fullscreen |
| Tab | Toggle Alternate assets |

### Graphics Backends
Currently, there are three rendering APIs supported: DirectX11 (Windows), OpenGL (all platforms), and Metal (macOS). You can change which API to use in the `Settings` menu of the menubar, which requires a restart.  If you're having an issue with crashing, you can change the API in the `Ghostship.cfg.json` file by finding the line `"Backend":{`... and changing the `id` value to `3` and set the `Name` to `OpenGL`. `DirectX 11` with id `2` is the default on Windows. `Metal` with id `4` is the default on macOS.

# Custom Assets
Custom assets are packed in `.o2r` or `.otr` files. To use custom assets, place them in the `mods` folder.

If you're interested in creating and/or packing your own custom asset `.o2r`/`.otr` files, check out the following tools:
* [**retro - OTR and O2R generator**](https://github.com/HarbourMasters64/retro)
* [**fast64 - Blender plugin**](https://github.com/HarbourMasters/fast64)

# Development
### Building

If you want to manually compile Ghostship, please consult the [building instructions](https://github.com/HarbourMasters/Ghostship/blob/develop/docs/building.md).

### Playtesting
If you want to playtest a continuous integration build, you can find them at the links below. Keep in mind that these are for playtesting only, and you will likely encounter bugs and possibly crashes. 

* [Windows](https://nightly.link/HarbourMasters/Ghostship/workflows/main/develop/Ghostship-windows.zip)
* [macOS](https://nightly.link/HarbourMasters/Ghostship/workflows/main/develop/Ghostship-mac.zip)
* [Linux](https://nightly.link/HarbourMasters/Ghostship/workflows/main/develop/Ghostship-linux.zip)

<a href="https://github.com/Kenix3/libultraship/">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/poweredbylus.darkmode.png">
    <img alt="Powered by libultraship" src="./docs/poweredbylus.lightmode.png">
  </picture>
</a>

# Special Thanks:

* [Kenix3](https://github.com/Kenix3/libultraship/) : for helping with the Engine development that were brought into other ports.
* [HM64 Team](https://github.com/harbourMasters) : for creating Libultraship and helping with various issues.
* [garrettjoecox](https://github.com/garrettjoecox) : for contributing with fixes to this port.
* [inspectredc](https://github.com/inspectredc) : for contributing to the extraction of the game's assets.
* [Malkierian](https://github.com/Malkierian) : for contributing to the extraction of the game's assets.
