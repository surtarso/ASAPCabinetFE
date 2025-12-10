<div align="center">
  
[![GitHub stars](https://img.shields.io/github/stars/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/surtarso/ASAPCabinetFE.svg?style=social)](https://github.com/surtarso/ASAPCabinetFE/network/members)
[![C++ Standard](https://img.shields.io/badge/C++-20-blue.svg?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.x-orange?logo=cmake&logoColor=white)](https://cmake.org/)
![imgui version](https://img.shields.io/badge/imgui-1.92.4-blue?logo=c%2B%2B&logoColor=white)
![vpin version](https://img.shields.io/badge/VPin-0.18.6-orange?logo=rust&logoColor=white)
![SDL 2 Version](https://img.shields.io/badge/SDL2-2.3x-red?logo=sdl&logoColor=white)
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
> Grab precompiled builds from [Releases](https://github.com/surtarso/ASAPCabinetFE/releases) or [Actions](https://github.com/surtarso/ASAPCabinetFE/actions) page for the fastest start.
>
> Please refer to the [User's Manual](https://github.com/surtarso/ASAPCabinetFE/blob/main/UsersManual.md)

---

### TL:DR <img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian
<details>
<summary>Copy/paste like a <b>senior systems administrator</b> who has meticulously tested the <i>Ctrl+C</i> and <i>Ctrl+V</i> functionality for three years before committing it to production.</summary>

```sh
sudo apt-get install git findutils -y -qq
git clone https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
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
git clone https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
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

### TL:DR <img src="https://twemoji.maxcdn.com/v/latest/svg/1f34e.svg" width="15"/> MacOS (experimental)
<details>
<summary>UNTESTED <b>arm64</b> build <img src="https://twemoji.maxcdn.com/v/latest/svg/1f37a.svg" alt="Beer" width="15"/> (I do NOT own a Mac and I just made it compile and link without errors, if you are a Mac person and interested in this, feel free to jump in as I have no real way to test this right now)
</summary>

```bash
# Install Homebrew if you don't have it:
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
git clone https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE && cd ASAPCabinetFE
git lfs install
git lfs pull
brew update
cat brew-packages.txt | xargs brew install
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${HOME}/ASAPCabinetFE -DCMAKE_PREFIX_PATH="$(brew --prefix)" -DCMAKE_FIND_ROOT_PATH="$(brew --prefix)" -DCMAKE_OSX_ARCHITECTURES="arm64" ..
cmake --build . -j$(sysctl -n hw.logicalcpu)
cmake --install .
```
</details>

## Front-End Features

| Topper/DMD without Metadata | Topper/DMD with Metadata |
|------------|----------|
| <img src="https://github.com/user-attachments/assets/cde5972e-80fe-4d2d-abd0-aaa41691a882" width="400"> | <img src="https://github.com/user-attachments/assets/933508dd-72e3-464f-9eae-2a651c8aaeed" width="400"> |

- Full screen multi monitor display of table playfield, backglass, topper and DMD.
- Tailored to mimic [Visual Pinball X](https://github.com/vpinball/vpinball) cabinet setup for seamless transitions.
- Use `VPinballX.ini` for size and position auto-configuration.
- Sort tables by title, type, manufacturer, year or author.
- Navigate tables with titles and/or wheels and marquees.
- Support for ambience, per-table music and launch sounds.
- Extended settings for many display configuration arrangements.
- Keybind friendly and joystick support.
- Extremely lightweight, blazing fast and fully customizable.
- No need to download artpacks, **generate** your own.
- Multiple animations to indicate your missing media or to use as default.
- Procedural generated DMDs with table title or manufacturer logos based on metadata info. (see [Legal](#legal))
- Use you own **128x32 real DMD** images _(animations not yet implemented)_
- Fully compatible with VPX Standalone _v10.8.0_ and _v10.8.1_

## Editor Features

| The Editor |
| ----- |
| <img src="https://github.com/user-attachments/assets/96d311ad-5027-4a48-b979-314e9e75f2e6" width="900" /> |

- Spreadsheet view of all owned tables.
- Row recolor on successful and failure launches for easy ID.
- Easily filter tables by any characteristics.
- View complete table metadata and art with a single click.
- Fuzzy search and play hotkey making tests super fast.
- Open root or specific folders straight from the editor.
- Lists user owned media (images, videos and sounds).
- Lists table specific files as INIs, VBSs, B2Ss.
- Lists table specific extras as altColor, altSound etc.
- Mark tables you have overrided metadata.
- Advanced menu for file operations (delete, backup, override metadata).
- Manage media cache with a single click.
- **Create** custom table overrides for display and matching.
- Advanced menu for [VpxTool](https://github.com/francisdb/vpxtool) --flags.
- Use [VPin](https://github.com/francisdb/vpin) to **retrieve** table file metadata.
- **Match** metadata with [VPSDb](https://virtual-pinball-spreadsheet.web.app/) API with visual feedback.
- Browse [VPSDb](https://virtual-pinball-spreadsheet.web.app/) to **find and download** new tables.
- Automatically apply or update table **patches** from [vpx-standalone-scripts](https://github.com/jsm174/vpx-standalone-scripts).
- Download **media** from the [Vpin Media Database](https://github.com/superhac/vpinmediadb) and [Launchbox Database](https://gamesdb.launchbox-app.com/games/results/?platform=pinball).
- Update check and notifications to keep you up to date.
- Update online databases on the fly.

>[!NOTE]
>Check the [User's Manual](UsersManual.md) for more details.

## Compiling and Running
<details open>
<summary>Make sure you have `git` and `xargs` available (<img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian)</summary>
  
```sh
sudo apt-get update
sudo apt-get install git findutils -y
```
</details>
<details>
<summary>Make sure you have `git` and `xargs` available (<img src="https://cdn.simpleicons.org/archlinux/1793D1" alt="Arch Linux" width="15"/> Arch)</summary>
  
```sh
sudo pacman -Syu git findutils --noconfirm
```
</details>

Clone the Repository
```sh
git clone https://github.com/surtarso/ASAPCabinetFE.git ASAPCabinetFE-src
cd ASAPCabinetFE-src
git lfs install
git lfs pull
```
<details open>
<summary>Install Dependencies (<img src="https://cdn.simpleicons.org/debian/CE0056" alt="Debian" width="15"/> Debian)</summary>
  
```sh
cat apt-packages.txt | xargs sudo apt-get install -y
```
</details>
<details>
<summary>Install Dependencies (<img src="https://cdn.simpleicons.org/archlinux/1793D1" alt="Arch Linux" width="15"/> Arch)</summary>
  
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
# or ./ASAPCabinetFE-Editor
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
# or ./ASAPCabinetFE-Editor
```
</details>

_Built something cool? Share it with us in the [Discussions](https://github.com/surtarso/ASAPCabinetFE/discussions)!_

## Planned Features:
We’re dreaming big! Help us bring these features to life:

- DOF support.
- Real DMD connection.
- Highscore tracking.
- Multi-launchers.
- In-game video recording.
- Media manipulation.
- Attract Mode.
- Custom collections.
- CLI automation.
- Tournaments. (maybe)

## In Development now:

- Flatpak packaging.
- Integration of [VPXGUITools](https://github.com/surtarso/vpx-gui-tools) for table management. (--editor)

The **new editor** is almost ready and can be *beta tested* with the [latest release](https://github.com/surtarso/ASAPCabinetFE/releases/)! Use it's own binary `ASAPCabinetFE-Editor` or start with `ASAPCabinetFE --editor` (or `-e` for short) to open it up. Please report any issues or suggestions! *(some functionalities are not yet implemented)*

https://github.com/user-attachments/assets/2a430103-da5d-49fc-95c0-c1ee10a280c1
<p align="center"><i>Example showing fuzzy search keyboard grab and table launching.</i></p>

## Contribute
- Contributions are very welcome! Check the [TODO](TODO) list, open issues or pull requests to help improve this app—every bit helps.
- Contributions to `128x32` logos are highly appreciated. Open pull requests to the `assets/img/dmd_still/` folder.
- If you're a *Dear ImGui* wizard, please contribute to UI polishing as I'm a complete loser for it, sorry.
- We're looking for a MacOS maintainer to add/keep/test the `__APPLE__` blocks. (Thanks [@herrmito](https://github.com/herrMirto)!)
- You can find the [documentation here](https://surtarso.github.io/ASAPCabinetFE/) or by typing `doxygen` on the project root, and opening docs/index.html.

## Acknowledgments
- [Visual Pinball X](https://github.com/vpinball/vpinball) team.
- [VPin](https://github.com/francisdb/vpin) / [VPXTool](https://github.com/francisdb/vpxtool) team.
- [Virtual Pinball Database](https://github.com/vpdb) team.
- [Vpin Media Database](https://github.com/superhac/vpinmediadb) team.
- [Launchbox Database](https://gamesdb.launchbox-app.com/) team.
- [VPForums](https://www.vpforums.org/) / [VPUniverse](https://vpuniverse.com/) team.
- A BIG THANK YOU to [@jsm174](https://github.com/jsm174/) [@francisdb](https://github.com/francisdb/) [@superhac](https://github.com/superhac/) [@Toxieainc](https://github.com/toxieainc) [@MikedaSpike](https://github.com/MikedaSpike).
- And a special thanks to _Snortt_ for bearing with me thru all of this.
- Thanks everyone who helped making Visual Pinball X for Linux a reality. Really, you guys rock!

## Footnote
If you need help installing and configuring [Visual Pinball X](https://github.com/vpinball/vpinball) check out [my wiki page](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux).

Join the [Discussions](https://github.com/surtarso/ASAPCabinetFE/discussions) page for questions and ideas!

## Legal
“All trademarks and logos are the property of their respective owners and are used only for identification purposes. This project is not affiliated with or endorsed by any company whose logo appears here. Manufacturer logos appear in low-resolution form strictly for identification of the associated tables.”
