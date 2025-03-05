# As Simple As Possible Cabinet Front-End (in C++)

This is a work in progress port of [asap-cabinet-fe](https://github.com/surtarso/asap-cabinet-fe) Python app to C++/SDL2.

## How it works

ASAP-CABINET-FE is a C++/SDL2 application that scans a specified folder for .vpx files and displays images for the playfield, wheel, backglass, and DMD. It creates two windows: primary (playfield) and secondary (backglass). Users can change tables using the left/right arrow keys and launch the table via an external process (VPinballX_GL) by pressing Enter.

## Features

- Scans `VPX_ROOT_FOLDER` recursively for .vpx files
- Loads images or videos for the playfield, wheel, backglass, and DMD for each table
- Creates two windows: primary (playfield) and secondary (backglass)
- Uses left/right arrow/shift keys to change tables with a fade transition
- Press Enter to launch the table via vpinballx_gl process

## Dependencies

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

## Required Libraries

Make sure the following libraries are installed:

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer
- VLC
