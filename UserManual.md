# As Simple As Possible Cabinet Front-End _User's Manual_

Welcome to ASAPCabinetFE! This is your go-to app for playing Visual Pinball X (VPX) tables with ease. This manual will walk you through how to use it—Browse tables, launching games, snapping screenshots, and tweaking settings—all without any techy stuff. Everything you need is right in the app’s config menu. Let’s dive in!

## What’s ASAPCabinetFE All About?

ASAPCabinetFE is a flexible frontend that lets you:

  - Browse and pick VPX tables with cool previews.
  - Fetch, edit and match metadata.
  - Find new tables using the catalog.
  - Launch tables straight into Visual Pinball X.
  - Take screenshots or videos of your tables to use as previews.
  - Adjust all settings using an in-app menu.

No complicated setup—just fun pinball action!

## Table of Contents

  * [Getting Started](#getting-started)
      * [First Things First: Table Structure](#first-things-first-table-structure)
      * [Starting the App for the First Time](#starting-the-app-for-the-first-time)
  * [Windows Positions](#windows-positions)
      * [Manual Positioning](#manual-positioning)
      * [Automatic Positioning](#automatic-positioning)
  * [Adding Your Own Previews](#adding-your-own-previews)
      * [How It Works](#how-it-works)
      * [Moving Around the Table List](#moving-around-the-table-list)
      * [Playing a Table](#playing-a-table)
  * [Taking Screenshots](#taking-screenshots)
      * [How to Start Screenshot Mode](#how-to-start-screenshot-mode)
      * [Snapping a Picture](#snapping-a-picture)
      * [Getting Out](#getting-out)
      * [Generate Videos](#generate-videos)
  * [Tweaking Stuff with the Configuration Menu](#tweaking-stuff-with-the-configuration-menu)
      * [Opening the Menu](#opening-the-menu)
      * [What Can You Change? (Everything!)](#what-can-you-change-everything)
      * [Saving Your Changes](#saving-your-changes)
  * [Controls (Key Bindings)](#controls-key-bindings)
      * [Basic Default Keys](#basic-default-keys)
      * [Changing Keys](#changing-keys)
  * [Sound Stuff](#sound-stuff)
  * [Metadata Editor](#metadata-editor)
  * [Virtual Spreadsheet Database](#virtual-pinball-spreadsheet-database-browser)
  * [The Scanners](#the-scanners)
  * [If Something Goes Wrong](#if-something-goes-wrong)
      * [No Tables Show Up](#no-tables-show-up)
      * [Screenshots/Videos Aren’t Saving](#screenshots-videos-arent-saving)
      * [No Sound](#no-sound)
      * [Videos Are Choppy Or Weird](#videos-are-choppy-or-weird)
      * [Windows Don't Move!?](#windows-dont-move)
      * [Tables Are Not Launching](#tables-are-not-launching)

## Getting Started

### First Things First: Table Structure
>[!IMPORTANT]
> This app expects you to organize your VPX tables in their own unique separate folders.

By default, the app looks for media files (images, audio and video) inside each table's folder:

![image](https://github.com/user-attachments/assets/e78f6296-c7fa-42cb-a966-93815370aafd)

  - `myVPXtablesDir/myVPXtable/images/`
  - `myVPXtablesDir/myVPXtable/video/`
  - `myVPXtablesDir/myVPXtable/audio/`

>[!NOTE]
> If you want to know more about setting up the propper folder structure for VPX, please read my [wiki](https://github.com/surtarso/vpx-gui-tools/wiki/Visual-Pinball-X-on-Debian-Linux#Structure).

You can set these paths using the in-app configuration menu. You can use your own media or generate it with the `generate_media.sh` tool (see "Taking Screenshots" section).

### Starting the App for the First Time

Simply double-click the ASAPCabinetFE icon (or start it as you would any other app). The first time you run it, a setup window will appear, asking you to point to your main VPX tables folder and the VPinballX program itself. Select these locations, save your choices, and close the menu. If everything is found correctly, the main app will then open.


![image](https://github.com/user-attachments/assets/a59cfb39-54f8-44fa-9730-aa2797b37303)


## Windows Positions
The first time the app opens you might see all windows on top of each other on the corner of a single monitor.
To ensure smooth transitions, you can set yout windows manually or let ASAPCab do it for you. You'll want the app's windows to align with your VPinballX setup.

### Manual Positioning

You can freely drag and place any of the app's windows wherever you prefer on your screen. Once you're happy with their positions, just **double-click anywhere on any screen to save these settings** to your configuration.

### Automatic Positioning

If you've already set up your window positions within VPinballX, and your `VPinballX.ini` file is in its default location (`~/.vpinball`), ASAPCabinetFE can automatically adopt those positions and sizes when it loads. This feature is enabled by default but can be toggled in the configuration menu.
>[!IMPORTANT]
>While this feature is enabled, manual positioning is disabled.

## Adding Your Own Previews

The app enhances your table browse experience with pictures or videos. You can easily add your own custom previews!

### How It Works

Place your media files directly into the respective table's folder using these specific filenames by default or change to whatever best fit your needs, as long as the files are inside `path/to/vpxTables/myVPXtable/ :

  - Wheel image: `images/wheel.png`
  - Playfield image: `images/table.png`
  - Backglass image: `images/backglass.png`
  - DMD image: `images/dmd.png`
  - Topper image: `images/topper.png`
  - Playfield video: `video/table.mp4`
  - Backglass video: `video/backglass.mp4`
  - DMD video: `video/dmd.mp4`
  - Topper video: `images/topper.mp4`
  - Table music: `audio/music.mp3`
    
The next time you scroll to a table, your custom media will appear. If you don't add anything, the app will use its default previews, which you can also customize in the configuration menu.

### Moving Around the Table List

Use the default keys (**Left Shift** and **Right Shift**) to navigate through your list of tables. As you browse, you'll see previews—like the playfield or backglass—appear on screen, helping you decide which table to play. You can also customize these keys in the in-app configuration menu.

### Playing a Table

Found a table you want to play? Just hit **Enter** to launch it directly in Visual Pinball X.
>[!NOTE]
>If a table fails to launch, the related log will be saved in the logs/ folder for inspection.

>[!IMPORTANT]
>This front-end has no relation to Visual Pinball X, please redirect any issues related to table launch to their maintainers.

## Taking Screenshots

Want to capture a cool image of your table? Screenshot mode has you covered.

### How to Start Screenshot Mode

1.  Navigate to the desired table in the list.
2.  Press **S**. This will launch the selected table in VPX and open a small, dedicated screenshot window.

### Snapping a Picture

While in screenshot mode, press **S** again to capture the visible screens (playfield, backglass, and any active DMD window). The captured images will automatically save to the respective table's folder, ready for use as previews.

### Getting Out

Finished capturing? Press **Q** to exit screenshot mode and close the table.

### Generate Videos

If still images aren't enough, you can use the `generate_media.sh` tool to record videos instead! Simply run `generate_media.sh --missing`. Depending on the size of your collection, this process might take some time, so feel free to take a break while it works.

## Tweaking Stuff with the Configuration Menu

You don't need to manually edit any config files; everything you want to change is accessible through the in-app configuration menu. The idea is for you to have the freedom to tweak anything you'd like to make the frontend your own.

### Opening the Menu

Press **C** at any time to bring up the configuration menu.

![image](https://github.com/user-attachments/assets/b1bb742b-7786-490e-bd50-a808bd4cdbc9)

### What Can You Change? (Everything!)

Here’s a glimpse of what you can tweak:

  - **Table Metadata**: Control information about your table, scan files, match metadata, sort.
  - **Title Display**: Customize table title, place stuff anywhere you like, or hide them entirely.
  - **UI Widgets**: Adjust or remove any on-screen overlay item.
  - **Audio Settings**: Independently adjust the volume for preview videos, table music, app sounds, and background music.
  - **Keybinds**: Customize the keyboard keys for various actions, and configure joystick support.
  - **DPI Settings**: Increase DPI scaling if elements appear too small on your screen.
  - **Window Settings**: Adjust the position and size of the playfield, backglass, or DMD windows.
  - **Media Dimensions**: Control the position and size of media elements within their respective windows, force images only.
  - **VPX Settings**: Set paths and control how you start VPinballX

Still not enough? Ok! You can also change ALL default media to create your own fallbacks. Use your own images or sounds for the UI to trully make this app yours.

### Saving Your Changes

Satisfied with your adjustments? Hit **Save**. If you change your mind and want to discard your modifications, press **Close** to exit the menu without saving.

## Controls (Key Bindings)

Here’s how you control the app using your keyboard to set things up:

### Basic Default Keys

  - **Left Shift**: Navigate to the previous table.
  - **Right Shift**: Navigate to the next table.
  - **Enter**: Launch the selected table.
  - **S**: Start screenshot mode.
  - **Q**: Exit screenshot mode, the configuration UI, or the application.
  - **C**: Toggle the configuration menu.

### Changing Keys

Want to use your joystick? But of'course! We have **cabinet** in the name, right? ;)


![image](https://github.com/user-attachments/assets/48f353e6-7ea7-4bba-8ac5-cf160949b16c)

1.  Press **C** to open the configuration menu.
2.  Navigate to the “Keybinds” section.
3.  Select the action you want to change (e.g., “Launch Table”) and then press the new key you wish to assign.

## Sound Stuff

Whether you love the tunes or prefer silence, you have full control over the audio:

![image](https://github.com/user-attachments/assets/7e8c27b2-7fec-41f8-8560-19a4d96902c8)

  - **Preview Videos**: Adjust the playback volume for video previews.
  - **Table Music**: Control the volume or mute individual table soundtracks.
  - **App Sounds**: Fine-tune the volume of interface sounds like clicks and navigation cues.
  - **Background Music**: Set the ambiance with adjustable background tunes.

You can find all these sound options within the “Audio Settings” section of the configuration menu.

## Metadata Editor

As you may have noticed, file metadata quality is really bad. But it doesn't have to be like that! Open the Metadata Editor (default: M) and save an override for you table info with correct information!

![image](https://github.com/user-attachments/assets/d2a0ec23-9324-4029-adbd-740a0a8333d5)

## Virtual Pinball Spreadsheet Database Browser

So, I heard you like pinball! That makes two of us! What if we could browse the VPS database from inside the app and get links straight to table downloads? Well, open the VPSDB Catalog (Default: N) and start browsing!

![image](https://github.com/user-attachments/assets/a4a7c171-920d-46c4-8041-288770a2f577)

## The Scanners

ASAPCabinetFE provides you with three distinct scanners:
- File Scanner: The first scanner that will run when you open the app. It will gather table information from the tree structure and file names. Detect table extras like altcolor, altmusic, altsound, ultraDMDs, pups...
- VPin Scanner: This is the scanner that will run when you select to use _metadata_. It will extract the metadata from your VPX files to display on the panel and possible help on matchmaking with vpsdb.
- VPSDb Scanner: Using all gathered information, toggling fetchVPSDB will attempt to match you metadata with the Virtual Pinball Spreadsheet metadata.

![image](https://github.com/user-attachments/assets/c42db096-bba6-4481-bf7d-958eb46f280b)

>[!NOTE]
> If you already have `vpxtool` installed or `vpxtool_index.json` already built in your tables folder, we will try to use it instead of VPin. No need to double scan or keep multiple metadata files.

## If Something Goes Wrong

### No Tables Show Up

Ensure your VPX tables are located in the correct directory as configured in the app (check the “Table Path” setting in the config menu). The app requires `.vpx` files to display a table list. Change the title source to 'filename' and force a rebuild to make sure _ASAPCab_ can see your files.

### Screenshots/Videos Aren’t Saving

Verify that the table's media folders (e.g., `images/`, `video/`) have the necessary write permissions to save new files. Try the external shell script tool.

### No Sound

Open the configuration menu (press **C**), go to “Audio Settings,” and check that nothing is muted or set too low. Table media sounds may differ when using other video backends.

### Videos Are Choppy Or Weird

Try changing the video backend setting in the configuration menu to one that might be better suited for your system.

### Windows Don't Move!?

If you are trying to set custom window positions and the App is not respecting you, uncheck "use VPinballX ini". This option will override your custom positions to use VPinballX positions instead.

### Tables Are Not Launching

The VPinballX binary logs will be redirected to the log folder.

**Please redirect VPX launching issues with logging information to the [VPinballX maintainers](https://github.com/vpinball/vpinball/issues/)** as I am also myself a mere user of the VPinballX engine.

>[!NOTE]
> Still encountering issues? Check the [project's discussion page](https://github.com/surtarso/ASAPCabinetFE/discussions) for solutions or open an [issue](https://github.com/surtarso/ASAPCabinetFE/issues) on the [project's repository](https://github.com/surtarso/ASAPCabinetFE) for assistance.
