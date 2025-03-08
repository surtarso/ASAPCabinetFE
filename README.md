# As Simple As Possible Cabinet Front-End (in C++)

This is a work in progress port of [asap-cabinet-fe](https://github.com/surtarso/asap-cabinet-fe) Python app to C++/SDL2.

## How it works

For now, check [the original project repo](https://github.com/surtarso/asap-cabinet-fe) for info. The aim here is to mimic and improve in areas where it failed, like playing more video formats and image types, more custumizable keys and better code logic overall.

<i>This port DOES NOT have a settings panel. Configure you paths in the **config.ini** file.</i>

## Features

- Scans `VPX_ROOT_FOLDER` recursively for .vpx files
- Loads images or videos for the playfield, wheel, backglass, and DMD for each table
- Creates two windows: primary (playfield) and secondary (backglass)
- Uses left/right arrow/shift keys to change tables with a fade transition
- Press Enter to launch the table via vpinballx_gl process

## Dependencies

Make sure the following libraries are installed:

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer
- VLC
  
Install the following dependencies:

```sh
sudo apt-get -y build-essential install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev
```

## Compilation

To compile the application, use the following command:

```sh
g++ main.cpp -std=c++17 -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc -o ASAPCabinetFE
```

For a specific setup, you can use:

```sh
g++ main.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc -o ASAPCabinetFE
```

## Usage

- Open **config.ini** and set your own paths.
- Run `./ASAPCabinetFE`

### Keymap (not yet configurable)

| Action             | Key Combination | Description                               |
|--------------------|-----------------|-------------------------------------------|
| Scroll Table (One) | Left/Right Shift | Move to the previous/next table.         |
| Scroll Table (Tens)| Left/Right Ctrl  | Move 10 tables backward/forward.           |
| Scroll by Letter   | Z and /         | Scroll tables alphabetically (previous/next). |
| Launch Table       | Enter           | Open the selected table.                 |
| Quit               | ESC/q           | Exit the table navigation/application.   |

## Generator tools:

These will record your screen to create media for the front end. PNG's or MP4's will be saved to where you configure in config.ini

Run them without args for help.

## Contribute

Contributions are very welcome. Feel free to open issues and pull requests.
