<h1 align="center">As Simple As Possible Cabinet Front-End</h1>

<p align="center">A multi-monitor <a href="https://github.com/vpinball/vpinball">VPinballX</a> front-end for your virtual pinball cabinet.</p>

<div align="center">
  <video src="https://github.com/user-attachments/assets/019e4170-94f2-464c-9209-4754ba87a029" width="400" />
</div>

<p align="center"><i>"As Simple As Possible".</i></p>

## Features
- Full screen multi monitor display of table playfield, backglass and DMD
- Navigate tables with titles and wheels
- Extended settings for many display configurations
- Extremely lightweight and simple
- No need to download artpacks, generate your own!*
- Just what it takes to make a cabinet look good

*Playfield and Backglass from automated screenshots. (See [tools](#generator-tools)).

## How it works

- Scans `VPX_ROOT_FOLDER` recursively for `.vpx` files.
- Loads images or videos for the playfield, wheel, backglass, and DMD for each table.
- Creates two windows: primary (playfield) and secondary (backglass).
- Uses left/right arrow/shift keys to change tables.
- Press Enter to launch the table via a `vpinballx_gl` process.

## Dependencies
> [!IMPORTANT]
> Ensure the following libraries are installed:
> - **SDL2**: Core library for graphics and input.
> - **SDL2_image**: Image loading support.
> - **SDL2_ttf**: Font rendering.
> - **SDL2_mixer**: Audio playback.
> - **VLC**: Video playback support.
> - **OpenGL**: Required for the `config` editor (via [ImGui](#install-imgui)).

### Installing Dependencies (Debian based)

For `ASAPCabinetFE`:
```sh
sudo apt-get update
sudo apt-get install -y build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev
```

For `config` (additional dependencies):
```sh
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev libglew-dev libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Compilation and Running

### 1. Clone the Repository
```sh
git clone https://github.com/surtarso/ASAPCabinetFE.git
cd ASAPCabinetFE
```

### 2. Compiling `ASAPCabinetFE` (Main Frontend)
Compile the main application:
```sh
g++ main.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc -o ASAPCabinetFE
```
> [!WARNING]
> Ensure `config.ini` is configured with your paths (see [Usage](#usage)).

- Run it:
  ```sh
  ./ASAPCabinetFE
  ```
  
### 3. Compiling `config` (Configuration Editor)

![image](https://github.com/user-attachments/assets/89efac26-cf71-454e-9559-171c44209160)

> [!TIP]
> This GUI contains tooltip explanations for all variables in config.ini.

#### Install ImGui
The `config` app uses ImGui, included as a submodule:
```sh
git submodule update --init --recursive
```

#### Compile
```sh
g++ config.cpp imgui/*.cpp imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl3.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -Iimgui -Iimgui/backends -lSDL2 -lGL -o config
```
- Run it:
  ```sh
  ./config
  ```
> [!NOTE]
> It loads `config.ini` from the current directory by default.

## Usage

1. **Configure Paths**:
   - Edit `config.ini` manually or use the `config` editor.
   - Set `VPX_ROOT_FOLDER` and paths for media (playfield, backglass, etc.).

2. **Run the Frontend**:
   ```sh
   ./ASAPCabinetFE
   ```

### Keymap (No`t Yet Configurable)
| Action             | Key Combination  | Description                               |
|--------------------|------------------|-------------------------------------------|
| Scroll Table (One) | Left/Right Shift | Move to the previous/next table.          |
| Scroll Table (Tens)| Left/Right Ctrl  | Move 10 tables backward/forward.          |
| Scroll by Letter   | Z and /          | Scroll tables alphabetically (prev/next). |
| Launch Table       | Enter            | Open the selected table.                  |
| Quit               | ESC or q         | Exit the table navigation/application.    |

## Generator Tools
These tools help you record your screen to create media (PNGs or MP4s) for the frontend, saved to paths specified in `config.ini`. Run them without arguments for help.

`./generate_media.sh` and `missing_media.sh`

> [!CAUTION]
> Make sure `config.ini` is properly configured before generating media.
## Troubleshooting
- **Compilation Fails**:
  - Verify all dependencies are installed.
  - Ensure the ImGui submodule is initialized (`imgui` directory should not be empty).
- **Runtime Errors**:
  - Check that `config.ini` exists and is readable/writable.
  - Install missing runtime libraries (e.g., `sudo apt-get install libsdl2-2.0-0`).
- **Other Platforms**:
  - *Windows*: Use MinGW/MSYS2 with equivalent libraries.
  - *macOS*: Use Homebrew (e.g., `brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer libvlc glew`).

## Contribute
Contributions are very welcome! Open issues or pull requests to help improve this app.

If you need help installing and configuring Visual Pinball X check my [wiki](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux) page, also check my [vpx-gui-tools](https://github.com/surtarso/vpx-gui-tools/) to ease the process of settings tables up.

PS: There is a [discontinued version of this frontend in Python](https://github.com/surtarso/asap-cabinet-fe).
