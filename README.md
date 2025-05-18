<h1 align="center">As Simple As Possible Cabinet Front-End</h1>

<p align="center">A multi-monitor <a href="https://github.com/vpinball/vpinball">VPinballX</a> front-end for your virtual pinball cabinet.</p>

<div align="center">
  <video src="https://github.com/user-attachments/assets/019e4170-94f2-464c-9209-4754ba87a029" width="400" />
</div>

<p align="center"><i>"As Simple As Possible".</i></p>

## TL:DR
```sh
sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev xdotool imagemagick ffmpeg
git clone --recurse-submodules --shallow-submodules https://github.com/surtarso/ASAPCabinetFE.git && cd ASAPCabinetFE
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./ASAPCabinetFE
```

## Features
- Full screen multi monitor display of table playfield, backglass and DMD.
- Tailored to mimic [Visual Pinball X](https://github.com/vpinball/vpinball) cabinet setup for seamless transitions.
- Use [vpxtool](https://github.com/francisdb/vpxtool) to retrieve table metadata. _(optional)_
- Navigate tables with titles and/or wheels.
- Extended settings for many display configuration arrangements.
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
> - **SDL2**: Core library for graphics and input.
> - **SDL2_image**: Image loading support.
> - **SDL2_ttf**: Font rendering.
> - **SDL2_mixer**: Audio playback.
> - **VLC**: Video playback support.
> - **xdotools**: Screen manipulation.
> - **ImageMagick**: Screen capture.
> - **FFMpeg**: Video assembly.

### Installing Dependencies (Debian based)
```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev xdotool imagemagick ffmpeg
```

### Compiling and Running

Clone the Repository
```sh
git clone --recurse-submodules --shallow-submodules https://github.com/surtarso/ASAPCabinetFE.git
cd ASAPCabinetFE
```

Build it
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

- Run it and configure your paths:
  ```sh
  ./ASAPCabinetFE
  ```

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

If you need help installing and configuring [Visual Pinball X](https://github.com/vpinball/vpinball) check out [my wiki page](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux), also check out [VPXGUITools](https://github.com/surtarso/vpx-gui-tools/) to help ease the process of settings tables up.

_PS: There is a [discontinued version](https://github.com/surtarso/asap-cabinet-fe) of this frontend in Python._
