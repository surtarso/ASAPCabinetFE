<div align="center">
  
[![GitHub stars](https://img.shields.io/github/stars/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/network/members)
[![C++ Standard](https://img.shields.io/badge/C++-20-blue.svg?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.x-orange?logo=cmake&logoColor=white)](https://cmake.org/)
[![Latest Compile](https://github.com/surtarso/ASAPCabinetFE/actions/workflows/compile_release.yml/badge.svg?branch=main)](https://github.com/surtarso/ASAPCabinetFE/actions/workflows/compile_release.yml)
[![Latest Release](https://img.shields.io/github/v/release/surtarso/ASAPCabinetFE)](https://github.com/surtarso/ASAPCabinetFE/releases)
<img alt="GitHub commits since latest release" src="https://img.shields.io/github/commits-since/surtarso/ASAPCabinetFE/latest">
[![License](https://img.shields.io/github/license/surtarso/ASAPCabinetFE.svg)](https://github.com/surtarso/ASAPCabinetFE/blob/main/LICENSE)
[![Documentation](https://img.shields.io/badge/Docs-Doxygen-blueviolet)](https://surtarso.github.io/ASAPCabinetFE/)
<img alt="GitHub repo size" src="https://img.shields.io/github/repo-size/surtarso/ASAPCabinetFE">
<img alt="GitHub Downloads (all assets, all releases)" src="https://img.shields.io/github/downloads/surtarso/ASAPCabinetFE/total">


</div>

<h1 align="center">As Simple As Possible Cabinet Front-End</h1>

<p align="center">A multi-monitor Virtual Pinball Suite for Linux.</p>
<p align="center">Yes! You <b>can</b> use on single monitor 🙃</p>

<div align="center">
  <video src="https://github.com/user-attachments/assets/019e4170-94f2-464c-9209-4754ba87a029" width="400" />
</div>

<p align="center"><i>"As Simple As Possible".</i></p>

> [!TIP]
> :godmode: Grab precompiled builds from [Releases](https://github.com/surtarso/ASAPCabinetFE/releases) or [Actions](https://github.com/surtarso/ASAPCabinetFE/actions) page for the fastest start. Please refer to the [User's Manual](https://github.com/surtarso/ASAPCabinetFE/blob/main/UserManual.md)

---

### TL:DR <img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian
<details>
<summary>Copy/paste like a <b>senior systems administrator</b> who has meticulously tested the <i>Ctrl+C</i> and <i>Ctrl+V</i> functionality for three years before committing it to production.</summary>

```sh
sudo apt-get install git findutils -y -qq
git clone --depth 1 https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
git lfs install && git lfs pull
cat apt-packages.txt | xargs sudo apt-get install -y
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
cmake --install .
cd ~/ASAPCabinetFE
./ASAPCabinetFE
```
</details>

### TL:DR <img src="https://cdn.simpleicons.org/archlinux/1793D1" alt="Arch Linux" width="15"/> Arch
<details>
<summary>Copy/paste like an <b>overcaffeinated trench-coat-wearing coder</b> whose keyboard is sticky with energy drink residue and whose <i>one true source</i> is an outdated Wiki page.</summary>
  
```sh
sudo pacman -Syu --noconfirm git findutils
git clone --depth 1 https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
git lfs install && git lfs pull
cat pacman-packages.txt | xargs sudo pacman -S --needed --noconfirm
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
cmake --install .
cd ~/ASAPCabinetFE
./ASAPCabinetFE
```

</details>

## Features

- Full screen multi monitor display of table playfield, backglass, topper and DMD.
- Tailored to mimic [Visual Pinball X](https://github.com/vpinball/vpinball) cabinet setup for seamless transitions.
- Use `VPinballX.ini` for size and position auto-configuration.
- Use [VPin](https://github.com/francisdb/vpin) to **retrieve** table file metadata.
- **Match** metadata with [VPSDb](https://virtual-pinball-spreadsheet.web.app/) API.
- Browse [VPSDb](https://virtual-pinball-spreadsheet.web.app/) to **find and download** new tables.
- **Create** custom table overrides for media or metadata.
- Automatically apply or update table **patches** from [vpx-standalone-scripts](https://github.com/jsm174/vpx-standalone-scripts).
- Download **media** from the [Vpin Media Database](https://github.com/superhac/vpinmediadb).
- Sort tables by title, type, manufacturer, year or author.
- Navigate tables with titles and/or wheels and marquees.
- Support for ambience, per-table music and launch sounds.
- Extended settings for many display configuration arrangements.
- Keybind friendly and joystick support.
- Extremely lightweight and fully customizable.
- No need to download artpacks, **generate** your own. (See [tools](#generator-tools))
- Fully compatible with VPX Standalone _v10.8.0_ and _v10.8.1_
- Just what it takes to make your cabinet look good!

### How it works
- Scans recursively for `.vpx` files.
- Optionally scan file metadata or use your existing `vpxtool_index.json`.
- Creates **up to four** windows for playfield, backglass, topper and DMD.
- Loads default or custom media for each table.
- Launches tables with desired VPX executable.

>[!NOTE]
>Check the [User's Manual](UserManual.md) for more details.

## Compiling and Running
<details open>
<summary>👫 Make sure you have `git` and `xargs` available (<img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian)</summary>
  
```sh
sudo apt-get update
sudo apt-get install git findutils -y
```
</details>
<details>
<summary>👫 Make sure you have `git` and `xargs` available (<img src="https://cdn.simpleicons.org/archlinux/1793D1" alt="Arch Linux" width="15"/> Arch)</summary>
  
```sh
sudo pacman -Syu git findutils --noconfirm
```
</details>

💏 Clone the Repository
```sh
git clone --depth 1 https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
git lfs install
git lfs pull
```
<details open>
<summary>👪 Install Dependencies (<img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian)</summary>
  
```sh
cat apt-packages.txt | xargs sudo apt-get install -y
```
</details>
<details>
<summary>👪 Install Dependencies (<img src="https://cdn.simpleicons.org/archlinux/1793D1" alt="Arch Linux" width="15"/> Arch)</summary>
  
```sh
cat pacman-packages.txt | xargs sudo pacman -S --needed --noconfirm
```
</details>

<details>
<summary>🐞 Build and Run (Debug)</summary>

For debugging, build with the `Debug` configuration to include symbols and run directly from the build folder.

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build -S .
cmake --build build -j$(nproc)
cd build
./ASAPCabinetFE
```
</details>

<details open>
<summary>🕹️ Build and Install (Release)</summary>
  
```sh
cmake -DCMAKE_BUILD_TYPE=Release -B build -S .
cmake --build build -j$(nproc)
cmake --install build
```

Run and configure your paths:
```sh
cd ~/ASAPCabinetFE
./ASAPCabinetFE
```
</details>

_Built something cool? Share it with us in the [Discussions](https://github.com/surtarso/ASAPCabinetFE/discussions)!_

---

### 🚨 For Hyprland users 🚨
>[!WARNING]
>**Window positioning won't work in Hyprland**
>
>Use `hyprctl clients` to check exact titles or classes.

<details>
<summary>You can add window rules like this</summary>
  
```
# ----------- ASAPCabinetFE + VPinballX
# Playfield
windowrule = workspace 4, title:Playfield
windowrule = fullscreen, title:Playfield
windowrule = workspace 4, title:^(Visual Pinball Player)$
windowrule = fullscreen, title:^(Visual Pinball Player)$

# ASAPCabinetFE Backglass
windowrule = workspace 3, title:Backglass
windowrule = move 45% 3%, title:Backglass
# VPinballX Backglass
windowrule = workspace 3, initialTitle:^(.*Backglass)$
windowrule = workspace 3, title:^(Visual Pinball - Backglass)$
windowrule = move 45% 3%, initialTitle:^(.*Backglass)$
windowrule = move 45% 3%, title:^(Visual Pinball - Backglass)$

# DMD/Score
windowrule = workspace 3, title:DMD
windowrule = move 45% 75%, title:DMD
windowrule = workspace 3, title:^(Visual Pinball - Score)$
windowrule = move 45% 75%, title:^(Visual Pinball - Score)$

# Screenshot window
windowrule = float, initialClass:^(ASAPCabinetFE)$, initialTitle:^(VPX Screenshot)$
windowrule = center, initialClass:^(ASAPCabinetFE)$, initialTitle:^(VPX Screenshot)$
windowrule = float, class:^(ASAPCabinetFE)$, title:^(VPX Screenshot)$
windowrule = center, class:^(ASAPCabinetFE)$, title:^(VPX Screenshot)$
```
</details>

---

## Default Keymap
Customize these keybinds to fit your cabinet’s controls—because every setup is unique.

| Action             |        Key       | Description                               |
|--------------------|------------------|-------------------------------------------|
| Scroll Table (One) | Left/Right Shift | Move to the previous/next table.          |
| Scroll Table (Tens)| Left/Right Ctrl  | Move 10 tables backward/forward.          |
| Scroll by Letter   | z and /          | Scroll tables alphabetically (prev/next). |
| Scroll Random      | r                | Move to a random table.                   |
| Launch Table       | Enter            | Launch the selected table with VPinballX  |
| Launch SShot mode  | s                | Launch the table in screenshot mode.      |
| Config Panel       | c                | Toggle config panel.                      |
| Save window pos.   | left doubleclick | Saves current window positions in config. |
| Metadata Editor    | m                | Toggle Metadata Editor in current table.  |
| VPSdb Catalog      | n                | Toggle VPSdb catalog for browsing tables. |
| Quit               | q                | Exit the application.                     |

## Generator Tools (<img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian)
> [!IMPORTANT]
> Make sure your paths are properly configured before generating media.

**1.** Use the screenshot tool to take planned screenshots

Start the table in "Screenshot Mode" (default: s) and follow the on-screen instructions.

**2.** Use the generator to record your screen and create media (PNGs or MP4s) for the frontend, saved to paths specified in `settings.json`.
```sh
./generate_media.sh --help
```

**3.** Use the missing media tool to find those sneaky tables without art.
```sh
./missing_media.sh --help
```
>[!WARNING]
>Use __--vpx-version__ depending on your setup, ex: `./generate_media.sh --vpx-version 10.8.1 -d -f`

## Planned Features:
We’re dreaming big! Help us bring these features to life:

- Multi-launchers.
- In-game video recording.
- Attract Mode.
- CLI helpers.
- Highscore tracking.

## In Development now:
Check out how the editor is looking using the latest build! Still a work in progress and some functionalities are not yet implemented. Use it's own binary `ASAPCabinetFE-Editor` or start with `ASAPCabinetFE --editor` (or `-e` for short) to open it up. Suggestions are highly appreciated!

- Integration of [VPXGUITools](https://github.com/surtarso/vpx-gui-tools) for table management. (--editor)

## Contribute
Contributions are very welcome! Check the [TODO](TODO) list, open issues or pull requests to help improve this app—every bit helps.
You can find the [documentation here](https://surtarso.github.io/ASAPCabinetFE/) or by typing `doxygen` on the project root, and opening docs/index.html.

## Acknowledgments
- [Visual Pinball X](https://github.com/vpinball/vpinball) team.
- [VPin](https://github.com/francisdb/vpin)/[VPXTool](https://github.com/francisdb/vpxtool) team.
- [Virtual Pinball Database](https://github.com/vpdb) team.
- [Vpin Media Database](https://github.com/superhac/vpinmediadb) team.
- [VPForums](https://www.vpforums.org/)/[VPUniverse](https://vpuniverse.com/) team.
- A BIG THANK YOU to [@jsm174](https://github.com/jsm174/) [@francisdb](https://github.com/francisdb/) [@superhac](https://github.com/superhac/) [@Toxieainc](https://github.com/toxieainc).
- And a special thanks to _Snortt_ for bearing with me thru all of this.
- Thanks everyone who helped making Visual Pinball X for Linux a reality. Really, you guys rock!

## Footnote
If you need help installing and configuring [Visual Pinball X](https://github.com/vpinball/vpinball) check out [my wiki page](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux).

Join the [Discussions](https://github.com/surtarso/ASAPCabinetFE/discussions) page for questions and ideas!
