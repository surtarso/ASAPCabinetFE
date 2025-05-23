<div align="center">
  
<!-- [![GitHub stars](https://img.shields.io/github/stars/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/network/members) -->
[![License](https://img.shields.io/github/license/surtarso/ASAPCabinetFE.svg)](https://github.com/surtarso/ASAPCabinetFE/blob/main/LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++-17-blue.svg?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.x-orange?logo=cmake&logoColor=white)](https://cmake.org/)
[![Release C++ Compile](https://github.com/surtarso/ASAPCabinetFE/actions/workflows/compile_release.yml/badge.svg?branch=main)](https://github.com/surtarso/ASAPCabinetFE/actions/workflows/compile_release.yml)
[![Artifacts](https://img.shields.io/badge/Artifacts-Releases-lightgrey)](https://github.com/surtarso/ASAPCabinetFE/actions)
[![Documentation](https://img.shields.io/badge/Docs-Doxygen-blueviolet)](https://surtarso.github.io/ASAPCabinetFE/)

</div>

<h1 align="center">As Simple As Possible Cabinet Front-End</h1>

<p align="center">A multi-monitor <a href="https://github.com/vpinball/vpinball">VPinballX</a> front-end for your virtual pinball cabinet.</p>

<div align="center">
  <video src="https://github.com/user-attachments/assets/019e4170-94f2-464c-9209-4754ba87a029" width="400" />
</div>

<p align="center"><i>"As Simple As Possible".</i></p>

## TL:DR
```sh
sudo apt-get install git findutils -y -qq
git clone --recurse-submodules --shallow-submodules https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
cat apt-packages.txt | xargs sudo apt-get install -y
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
cmake --install .
cd ~/ASAPCabinetFE
./ASAPCabinetFE
```

> [!NOTE]
> You can **download already compiled builds** directly from the [Actions page](https://github.com/surtarso/ASAPCabinetFE/actions).
> This is the quickest way to get started if you don't want to compile from source.
> Select the latest successful run and scroll down to the **"Artifacts"** section.
> Make sure you have runtime dependencies installed _(vlc ffmpeg xdotool imagemagick libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-ttf-2.0-0 libsdl2-mixer-2.0-0)_.

## Features
- Full screen multi monitor display of table playfield, backglass and DMD.
- Tailored to mimic [Visual Pinball X](https://github.com/vpinball/vpinball) cabinet setup for seamless transitions.
- Use [vpxtool](https://github.com/francisdb/vpxtool) to retrieve table metadata. _(optional)_
- Navigate tables with titles and/or wheels.
- Extended settings for many display configuration arrangements.
- Software and hardware rendering.
- Keybind friendly and joystick support.
- Extremely lightweight and customizable.
- No need to download artpacks, generate your own. (See [tools](#generator-tools))
- Just what it takes to make your cabinet look good!

## How it works
- Scans recursively for `.vpx` files.
- Creates up to three windows for playfield, backglass and DMD.
- Loads default or custom images and/or videos for the playfield, wheel, backglass, and DMD for each table.
- Launches tables with desired VPX executable.

### Dependencies
> [!IMPORTANT]
> Ensure the following libraries are installed:
 - **SDL2**: Core library for graphics and input.
 - **SDL2_image**: Image loading support.
 - **SDL2_ttf**: Font rendering.
 - **SDL2_mixer**: Audio playback.
 - **VLC**: Software decoding.
 - **FFMpeg**: Video assembly/playback.
 - **xdotools**: Screen manipulation.
 - **ImageMagick**: Screen capture.

### Compiling and Running

Make sure you have `git` and `xargs` available
```sh
sudo apt-get update
sudo apt-get install git findutils -y
```

Clone the Repository
```sh
git clone --recurse-submodules --shallow-submodules https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
```

Install Dependencies (Debian based)
```sh
cat apt-packages.txt | xargs sudo apt-get install -y
```

Build and install it
```sh
cmake -DCMAKE_BUILD_TYPE=Release -B build -S .
cmake --build build -j$(nproc)
cmake --install build
```

Run it and configure your paths:
```sh
cd ~/ASAPCabinetFE
./ASAPCabinetFE
```

> [!NOTE]
> The `Debug` build is meant to be run in /build and has no `--install` target.


### Default Keymap
| Action             |        Key       | Description                               |
|--------------------|------------------|-------------------------------------------|
| Scroll Table (One) | Left/Right Shift | Move to the previous/next table.          |
| Scroll Table (Tens)| Left/Right Ctrl  | Move 10 tables backward/forward.          |
| Scroll by Letter   | z and /          | Scroll tables alphabetically (prev/next). |
| Scroll Random      | r                | Move to a random table.                   |
| Launch Table       | Enter            | Launch the selected table with VPinballX  |
| Config             | c                | Toggle config panel.                      |
| Launch SShot mode  | s                | Open the table in screenshot mode.        |
| Take Screenshot    | s                | Takes a shot while in Screenshot mode.    |
| Quit SShot mode    | q                | Quit SShot mode and return to launcher.   |
| Quit               | q                | Exit the application.                     |
| Save window pos.   | left doubleclick | Saves current window positions in config. |

## Generator Tools
> [!WARNING]
> Make sure your paths are properly configured before generating media.

**1.** Use the screenshot tool to take planned screenshots

Start the table in "Screenshot Mode" (default: s) and follow the on-screen instructions.

**2.** Use the generator to record your screen and create media (PNGs or MP4s) for the frontend, saved to paths specified in `config.ini`.
```sh
./generate_media.sh --help
```

**3.** Use the missing media tool to find those sneaky tables without art.
```sh
./missing_media.sh --help
```

## Troubleshooting
- **Compilation Fails**:
  - Verify all dependencies are installed.
  - Ensure the ImGui submodule is initialized (`imgui` directory should not be empty).
- **Runtime Errors**:
  - Check that `config.ini` exists and is readable/writable.
  - Install missing runtime libraries (e.g., `sudo apt-get install libsdl2-2.0-0`).

## Planned Features:
- Music.
- Dynamic custom window creation for PuP, Toppers etc.
- Single table overrides for settings.
- Seamless switch with [VPXGUITools](https://github.com/surtarso/vpx-gui-tools) for table management.

## Contribute
Contributions are very welcome! Check the TODO list, open issues or pull requests to help improve this app.
You can create doxygen documentation by typing `doxygen` on the project root, and opening docs/index.html.

### FOOTNOTE
If you need help installing and configuring [Visual Pinball X](https://github.com/vpinball/vpinball) check out [my wiki page](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux), also check out [VPXGUITools](https://github.com/surtarso/vpx-gui-tools/) to help ease the process of settings tables up.

_PS: There is a [discontinued version](https://github.com/surtarso/asap-cabinet-fe) of this frontend in Python._
