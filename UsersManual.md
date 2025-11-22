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
      * [Patching tables with latest standalone VBScripts](#patching-tables-with-latest-standalone-vbscripts)
  * [Windows Positions](#windows-positions)
      * [Manual Positioning](#manual-positioning)
      * [Automatic Positioning](#automatic-positioning)
  * [Adding Your Own Previews](#adding-your-own-previews)
      * [How It Works](#how-it-works)
      * [Default Previews](#default-previews)
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
      * [Screenshots And Videos Are Not Saving](#screenshots-and-videos-are-not-saving)
      * [No Sound](#no-sound)
      * [Videos Are Choppy Or Weird](#videos-are-choppy-or-weird)
      * [Windows Don't Move](#windows-dont-move)
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

<img width="808" height="508" alt="image" src="https://github.com/user-attachments/assets/824f8b36-f16a-4c07-87d3-053aa72358a5" />

### Patching Tables With Latest Standalone VBScripts
Enabling the auto-patcher will automatically download the latest patches from [vpx-standalone-scripts](https://github.com/jsm174/vpx-standalone-scripts) for the tables needing it.
You may keep this enabled to make sure your tables always have the latest patch.

## Windows Positions
The first time the app opens you might see all windows on top of each other on the corner of a single monitor.
To ensure smooth transitions, you can set yout windows manually or let ASAPCab do it for you. You'll want the app's windows to align with your VPinballX setup.

>[!NOTE]
>You can set a custom path for your VPinballX.ini in case you dont use the default location (optional).

### Manual Positioning

You can freely drag and place any of the app's windows wherever you prefer on your screen. Once you're happy with their positions, just **double-click anywhere on any screen to save these settings** to your configuration.

### Automatic Positioning

If you've already set up your window positions within VPinballX, and your `VPinballX.ini` file is in its default location (`~/.vpinball`), ASAPCabinetFE can automatically adopt those positions and sizes when it loads. This feature is not enabled by default but can be toggled in the configuration menu.

>[!IMPORTANT]
>While this feature is enabled, manual positioning is disabled.

<img width="730" height="720" alt="image" src="https://github.com/user-attachments/assets/22534397-ac84-4f1e-8206-3fdd43c217ad" />

>[!WARNING]
>In Hyprland, positioning will be handled by Hyprland's window rules.

### Media sizing

Media and windows are configured separatedly. Most cabinets will have their monitors using bezels and/or frames, so you are free to position, size and rotate the media itself inside the chosen window size. **This is specially important if your playfield is not in portrait mode, since you will need to rotate the media.**

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
    
The next time you scroll to a table, your custom media will appear. 

### Default previews

If you don't add anything, the app will use its default animations, which you can choose in the configuration menu between "No Media" animations for collectionists, or "computer generated" DMD's for Topper and DMD windows that will use the table metadata to display info and some placeholders for backglass and playfield!

#### Default animations without metadata

<img width="1032" height="263" alt="No Media default display" src="https://github.com/user-attachments/assets/1517e21b-be1d-4d64-ba8a-0253f7dd1981" />
<p align="center"><i>Example 'No Media' default display animation on DMD screen</i></p>

<img width="1028" height="263" alt="image" src="https://github.com/user-attachments/assets/ba0ad301-a782-443a-bffb-d57d63646025" />
<p align="center"><i>Example 'Generated' DMD without metadata display on DMD screen</i></p>

#### Default animations with metadata

>[!IMPORTANT]
>Logos are mapped to images comparing metadata 'manufacturer' field with /dmd_still/ filenames. If it can't match, it will type what's in 'manufacturer' like below. If that field is empty you get 'INSERT COINS' as above. To see those you need top opt-in or the default animation is shown. (first image)

<img width="1034" height="268" alt="Generated DMD metadata display" src="https://github.com/user-attachments/assets/a089740d-431a-41d3-9dfb-7d044818eed0" />
<p align="center"><i>Example 'Generated' DMD using metadata for display animation on DMD screen</i></p>

<img width="1032" height="263" alt="image" src="https://github.com/user-attachments/assets/d675d28b-f54d-4499-a17e-c5a712091384" />
<p align="center"><i>Example 'Generated' DMD using metadata with logo display animation on DMD screen</i></p>

>[!NOTE]
>The preference will always be Custom Videos -> Custom Images -> Default Animations. You can skip videos by using the "Images Only" option in the configuration menu.

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

<img width="733" height="356" alt="image" src="https://github.com/user-attachments/assets/d11cdf2f-0de0-464a-bab7-3cb07d4505dd" />

### What Can You Change? (Everything!)

Here’s a glimpse of what you can tweak:

  - **Table Metadata**: Control information about your table, scan files, match metadata, sort.
  - **Title Display**: Customize table title, place stuff anywhere you like, or hide them entirely.
  - **UI Widgets**: Adjust or remove any on-screen overlay item.
  - **Audio Settings**: Independently adjust the volume for preview videos, table music, app sounds, and background music.
  - **Keybinds**: Customize the keyboard keys for various actions, and configure joystick support.
  - **DPI Settings**: Increase DPI scaling if elements appear too small on your screen.
  - **Window Settings**: Adjust the position and size of the playfield, backglass, dmd or topper windows.
  - **Media Dimensions**: Control the position and size of media elements within their respective windows, force images only.
  - **VPX Settings**: Set paths and control how you start VPinballX

### Saving Your Changes

Satisfied with your adjustments? Hit **Save**. If you change your mind and want to discard your modifications, press **Close** to exit the menu without saving. (Or use the keybind 'q' to quit or 'c' to toggle without saving.)

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

<img width="730" height="418" alt="image" src="https://github.com/user-attachments/assets/b2e173a6-7b5b-4632-9f84-192b487b703e" />

1.  Press **C** to open the configuration menu.
2.  Navigate to the “Keybinds” section.
3.  Select the action you want to change (e.g., “Launch Table”) and then press the new key you wish to assign.

## Sound Stuff

Whether you love the tunes or prefer silence, you have full control over the audio:

<img width="730" height="222" alt="image" src="https://github.com/user-attachments/assets/a320aeaf-2640-4706-b673-689a0ecc08e1" />

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

<img width="729" height="346" alt="image" src="https://github.com/user-attachments/assets/146e67ad-68f5-4c00-b42a-6e420b5993b2" />

Usage:
- The default behavior is to *only read the index* for a fast start.
- If an index can't be found in the default path, it will fire the *file scanner* to create one.
- You can opt in the other scanners as you wish, but notice some features will require all scanners to have ran at least once, like fetching art from VPin Database.
- Your files will be re-scanned (for changes, new tables etc) on every start-up if you opt to uncheck 'fast start'.
- If you want to rebuild the database from scratch (overwrite instead of increment) you need to check 'force rebuild' (and uncheck 'fast start').

>[!NOTE]
> If you already have `vpxtool` installed or `vpxtool_index.json` already built in your tables folder, we will try to use it instead of VPin. No need to double scan or keep multiple metadata files.

>[!TIP]
> Table management is easier using the built-in editor, start with (`ASAPCabinetFE-Editor`).

<img width="1288" height="727" alt="ASAPCabinetFE Editor" src="https://github.com/user-attachments/assets/320e5c61-ea1f-4e85-ac96-729ea7cc7afa" />

## If Something Goes Wrong

### No Tables Show Up

Ensure your VPX tables are located in the correct directory as configured in the app (check the “Table Path” setting in the config menu). The app requires `.vpx` files to display a table list. Change the title source to 'filename' and force a rebuild to make sure _ASAPCab_ can see your files.

### Screenshots And Videos Are Not Saving

Verify that the table's media folders (e.g., `images/`, `video/`) have the necessary write permissions to save new files. Try the external shell script tool.

### No Sound

Open the configuration menu (press **C**), go to “Audio Settings,” and check that nothing is muted or set too low. Table media sounds may differ when using other video backends.

### Videos Are Choppy Or Weird

Try changing the video backend setting in the configuration menu to one that might be better suited for your system.

### Windows Don't Move

If you are trying to set custom window positions and the App is not respecting you, uncheck "use VPinballX ini". This option will override your custom positions to use VPinballX positions instead.

### Tables Are Not Launching

The VPinballX binary logs will be redirected to the log folder.

**Please redirect VPX launching issues with logging information to the [VPinballX maintainers](https://github.com/vpinball/vpinball/issues/)** as I am also myself a mere user of the VPinballX engine.

>[!NOTE]
> Still encountering issues? Check the [project's discussion page](https://github.com/surtarso/ASAPCabinetFE/discussions) for solutions or open an [issue](https://github.com/surtarso/ASAPCabinetFE/issues) on the [project's repository](https://github.com/surtarso/ASAPCabinetFE) for assistance.
