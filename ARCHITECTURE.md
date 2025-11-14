### "As Simple As Possible" Cabinet Front End
### *— Full System Architecture Overview*

---

# **1. Introduction**

ASAPCabinetFE is a **C++20**, **SDL2**, **ImGui**, and **VLC/FFmpeg–powered** frontend designed for media-rich cabinet environments (pinball cabinets, arcade setups, multi-display rigs).
It offers:

* A **runtime Front-end Mode** for standard use
* A **rich Editor Mode** featuring metadata editing, previews, and table management
* A **modular architecture** based on interfaces and dependency inversion
* Extensible video/audio backends (FFmpeg, VLC, SDL-drawing fallback, dummy players)
* An index + metadata system to unify disparate table sources (local VPX file metadata + VPSDB metadata)

This document explains:

* The big-picture system design
* Folder structure
* Major subsystems
* Flow of control in both modes
* Rendering pipeline
* Media subsystem
* Table indexing and metadata system
* UI architecture
* Event & threading model
* Dependency injection model
* Opportunities for expansion

This is the **systems-level** documentation that complements Doxygen’s class-level diagrams.

---

# **2. High-Level Architecture**

ASAPCabinetFE is composed of these core domains:

1. **Application Layer**
   * App mode (`app.cpp`)
   * Editor mode (`editor.cpp`)
   * Input mode handling
   * Main loop

2. **Core Framework**
   * Dependency Factory
   * Interfaces (`IConfigService`, `IRenderer`, `IAssetManager`, etc.)
   * UI manager
   * Modal dialog controller

3. **Rendering Subsystem**
   * SDL2-based rendering
   * Texture management
   * View layout
   * Playfield/backglass/DMD/Topper layouts
   * Title and text rendering helpers

4. **Media Subsystem**
   * `IVideoPlayer` abstraction
   * FFmpeg backend
   * VLC backend
   * Dummy backend (debug)
   * Default media renderer (SDL draw)
   * VideoPlayerFactory
   * VideoPlayerCache

5. **Asset System**
   * AssetManager (loading textures, videos, images, table media)
   * Management of wheel images, specific window assets
   * Media source (SDL draw, user image/video)

6. **Table System**
   * `ITableLoader` abstraction
   * Table loader
   * Table metadata merging
   * AsapIndex (built from local & VPSDB sources)
   * Local override system
   * JSON-based index management

7. **Editor System**
   * Panels for metadata editing
   * Table override editor
   * Media preview window
   * File operation actions

8. **UI System**
   * ImGui Manager
   * Panels and layout logic
   * Modal dialogs
   * Tooltips, color themes, AA/renderer settings
   * Search & filtering

9. **Configuration System**
   * `IConfigService` abstraction
   * App Settings
   * Editor Settings
   * ConfigService
   * JSON config reading/writing

10. **Launching / Execution System**
    * Table launcher
    * Process management
    * InputManager binding for app mode

11. **Window System**
    * `IWindowManager` abstraction
    * OrientationMode (Portrait/Landscape)
    * DPI scaling
    * Screen size detection
    * Multi-monitor support
    * Display-specific render targets

12. **Loading System**
    * Loading screen panel
    * Background worker threads
    * Loading jobs executed before the UI becomes visible
    * Progress/percentage updates
    * Asset preload phase

13. **File Operations / Async Actions**
    * File copy/delete
    * Preview refresh
    * Generating thumbnails
    * Moving assets
    * Scanning folders
    * Triggering shell commands (VPXTool)
    * VPin MDB image downloading

---

# **3. Project Folder Structure**

```
src/
├── capture/                        # Screenshot module
├── config/                         # Main configuration
│   └── ui/                         # Settings UI
├── core/                           # Front-end orchestration
│   ├── ui/                         # Front-end UI
├── editor/                         # Editor orchestration
│   └── ui/                         # Editor UI
├── keybinds/                       # Keybind and input managers
├── launcher/                       # VPX table launcher
├── log/                            # Logging
├── render/                         # Render pipeline
│   ├── assets/                     # Assets management
│   └── video_players/              # Video player factory
│       ├── ffmpeg/                 # Ffmpeg implementation
│       └── vlc/                    # VLC implementation
├── sound/                          # Sound subsystem
├── tables/                         # Table loaders
│   ├── overrides/                  # Metadata overrides
│   ├── vpinmdb/                    # Image download
│   ├── vpsdb/                      # vpsdb table linker
├── utils/                          # Path, OS, sha etc
├── vpin_ffi_wrapper/               # VPin (rust) wrapper
│
└── main.cpp                        # Main entry point for both modes
```

---

# **4. Application Entry Points**

There are **two separate entry modes**, both built from `main.cpp`.

### **4.1 Front-End Mode (`app.cpp`)**

Used for regular use. Playing tables.

Responsible for:

* Booting UI manager
* Creating the renderers
* Loading assets
* Handling input through InputManager
* Running the main render loop
* Displaying table title / wheel / previews
* Launching tables
* Managing video playback

### **4.2 Editor Mode (`editor.cpp`)**

Used for file & metadata management.

Responsible for:

* Advanced UI panels
* Table metadata and overrides editor
* Media preview (playfield/backglass/videos)
* Table browser with advanced filters
* Editing tool conditions

### **4.3 Both modes share:**

* Renderer subsystem
* Media subsystem
* Asset system
* Table system
* Dependency Factory
* Configuration

---

# **5. Main Loop and System Flow**

Both modes operate with:

1. Initialization
2. Dependency creation
3. Asset subsystem initialization
4. UI Manager loop
5. Rendering
6. Cleanup

**ImGui** drives the visible interface.
**SDL** drives the window/input/render loop.
**AssetManager** ensures media is available.
**TableLoader** + **AsapIndex** supply data to UI.
**VideoPlayers** stream media into textures.

---

# **6. Core Subsystem: Dependency Factory**

### **Purpose**

Centralizes the construction and wiring of:

* ConfigService
* Renderer
* AssetManager
* TableLoader
* Tables Index Manager
* VPsDB catalog manager
* VideoPlayerFactory
* VideoPlayerCache

Implements the **Dependency Inversion Principle**, ensuring all systems depend on small interfaces, not concrete classes.

### **Benefits**

* Great testability
* Very extensible
* Clean system boundaries
* Easy swapping of implementations

---

# **7. Rendering Subsystem**

### **7.1 Renderer Abstraction**

Located in:

```
src/render/
```

Provides:

```
IRenderer
IAssetManager
IVideoPlayer
```

### **7.2 SDL Renderer Implementation**

Implements:

* Hardware-accelerated textures
* Font/text drawing
* Viewports
* Multi-display layouts

### **7.3 Texture Flow**

Textures originate from:

1. Image files (wheel, playfield, backglass)
2. Generated assets (title text)
3. Generated animations (default media)
4. Video frames (FFmpeg/VLC)

AssetManager → Renderer → SDL_Texture → UI panels

### **7.4 Rendering Targets**

Depending on orientation:

* Landscape
* Portrait
* Multi-monitor rigs

### **7.5 Display Layout & Window behavior**

* Portrait vs landscape
* Multi-panel resizing
* Backglass placement
* DMD placement
* Topper support
* Multiple display indexes
* Window size constraints
* DPI scaling
* Fullscreen toggles

---

# **8. Media Subsystem (Video/Audio)**

### **8.1 Overview**

Media playback uses the `video_player_factory` with multiple backend implementations.

### **8.2 Backends**

Located under:

```
src/render/video_players
```

Backends:

1. `FFmpegPlayer`
2. `VlcVideoPlayer` (supports VLC v3/v4)
3. `DefaultMediaPlayer` (fallback animated content)
4. `DummyPlayer`  (debug)
5. `None` mode

### **8.3 VideoPlayerFactory**

Creates appropriate backend depending on settings:

* `VideoBackend::FFmpeg`
* `VideoBackend::VLC`
* `VideoBackend::Default`
* `VideoBackend::Dummy`

### **8.4 VideoPlayerCache**

Caches multiple video players to avoid expensive recreation during table preview transitions.

### **8.5 Media Flow**

```
AssetManager
    → requests player from VideoPlayerFactory
    → caches via VideoPlayerCache
    → updates frame (per render cycle)
    → provides SDL_Texture to Renderer/UI
```

### **8.6 Audio**

Audio is delegated to backend implementations (FFmpeg / VLC).
Settings (volume, mute) applied through ConfigService using pulseaudio.

---

# **9. Asset Subsystem**

### **9.1 AssetManager**

One of the core components.

Responsible for:

* Loading **textures**:

  * Wheel image
  * Playfield image
  * Backglass image
  * DMD image
  * Topper image

* Loading **video players**:

  * Playfield video
  * Backglass video
  * DMD video
  * Topper video

* Rendering **titles**:

  * AssetManager uses TitleRenderer to generate text textures.

* Color themes / styled assets:

  * Default fallback images
  * Placeholder media
  * Theme-specific icons

* Resolving file paths via `PathUtils`

* Applying table-specific overrides (custom / default)

* Managing lifecycle & cleanup of media assets

### **9.2 Asset Lifetime**

Assets are retained until:

* Table selection changes
* UI mode changes
* Renderer reinitializes
* Explicit cleanup

---

# **10. Table System**

### **10.1 TableLoader**

Loads:

* File structure information
* Local VPX file metadata
* VPsDB metadata
* User overrides
* Custom artwork paths
* Defaults (global settings)

### **10.2 AsapIndex**

Stored and accessed through:

```
src/tables/asap_index_manager.cpp
```

Provides:

* Table list
* Filtered index
* Search queries
* Metadata lookup

### **10.3 VPsDB Integration**

Handles:

* Catalog JSON
* VPsDB structure merging
* Author/Category/etc. fields

### **10.4 Table Data Structure**

Contains:

* Table paths
* Media paths
* Metadata
* Play options
* User overrides
* Launch settings

---

# **11. Editor System**

### **11.1 Purpose**

Provides advanced metadata editing tools not displayed in App mode.

### **11.2 Panels**

Panels include:

* Fuzzy Search
* Table details panel
* Table media preview panel
* Metadata editor panel
* File operations

### **11.3 Modal Dialogs**

Used for:

* Confirmations
* Warnings
* Error messages
* Asynchronous operations

Modal state lives in:

```
src/core/ui/modal_dialog
```

---

# **12. UI System**

### **12.1 ImGui Manager**

Centralizes:

* Theme creation
* Layouting
* Docking behavior
* Frame lifecycle
* Input routing
* Child panels
* Backgrounds, titles, dividers
* Loading screen
* Loading animation
* Status text
* Progress

### **12.2 Panels & Views**

Panels live in:

```
src/config/ui/    # The main configuration panel
src/core/ui/      # Front-end related panels
src/editor/ui/    # Editor related panels

#shared panels
src/tables/vpdsb/       # Vpsdb browser
src/tables/overrides/   # Metadata override
```

Each panel implements a `draw()` method following ImGui immediate-mode flow.

### **12.3 Tooltips**

Generated automatically from metadata fields and UI hints.

Tooltips live in:

```
src/util/editor_tooltips.h       # Editor buttons
src/config/settings.h            # Configuration panel variables
src/editor/ui/editor_header.cpp  # Editor spreadsheet entries
```

### **12.4 Search & Filtering**

Handled in Editor UI logic + table index management.
Uses fuzzy search logic to find tables by filename, table name, rom, manufacturer, year etc

---

# **13. Configuration System**

### **13.1 ConfigService**

Interface:

* Loads settings
* Persists changes
* Provides typed accessors
* Defines default values

### **13.2 Settings JSON**

Contains:

* UI preferences
* Video backend
* Playback settings
* Orientation mode
* Table paths
* Audio volume
* Performance settings
* Feature flags

---

# **14. Event & Threading Model**

### **14.1 Main Thread**

Handles:

* SDL events
* UI drawing
* Texture submission
* Video frame sampling
* Asset updates
* Rendering

### **14.2 Video Threads**

Backends (FFmpeg / VLC) run their own decode threads.
Synchronization handled by backend implementations.

### **14.3 Modal Thread Safety**

ModalDialog, loading screen, file operations use mutexes for consistency.

---

# **15. Launching System**

Located in:

```
src/launcher/
```

Responsible for:

* Constructing launch command line
* Starting external VPX processes
* Monitoring when child exits
* Restoring UI visibility afterward

**App Mode** launches tables via InputManager navigation.
**Editor Mode** launches tables directly from the launcher panel.

---

# **16. Error Handling & Logging**

### **Logging**

Centralized under:

```
log/logging.h
```

Used across:

* Media subsystem
* AssetManager
* TableLoader
* Renderer
* UI operations
* Loading feedback

### **Error Handling**

Uses:

* Logging
* Modal dialogs
* Immediate error overlays

---

# **17. Opportunities for Clean Refactoring**

These items improve maintainability before scaling the project.

### **17.1 Extract MediaManager**

Move video logic out of AssetManager into a dedicated subsystem.

### **17.2 Split IAssetManager**

Divide into:

* ITextureManager
* IMediaManager
* ITableAssetResolver

### **17.3 Introduce RenderFrame DTO**

One struct passed to UI with all needed textures.

### **17.4 Reduce TableData size**

Split into small structs:

* TableMediaInfo
* TablePaths
* TableMetadata
* TableStats

---

# **18. Example Diagram: High-Level Flow**

```
         ┌────────────────────────────────────┐
         │              main.cpp              │
         └────────────────────────────────────┘
                         │
      ┌──────────────────┴────────────────────┐
      │                                       │
┌───────────────┐                     ┌────────────────┐
│    app.cpp    │                     │   editor.cpp   │
└───────────────┘                     └────────────────┘
      │                                       │
      ├─────────────── create via ────────────┤
      │           DependencyFactory            │
      ▼                                       ▼
┌───────────────┐                     ┌────────────────┐
│   Renderer     │ ← SDL2 → textures │    UI Manager   │
└───────────────┘                     └────────────────┘
      │                                       │
      ▼                                       ▼
┌───────────────┐                     ┌────────────────┐
│ AssetManager  │                     │ Editor Panels  │
│ (future: MM)  │                     └────────────────┘
└───────────────┘
      │
      ▼
┌───────────────┐
│ MediaSystem   │ (VideoPlayerFactory, Cache)
└───────────────┘
      │
      ▼
┌───────────────┐
│ VideoPlayers  │ (FFmpeg/VLC/Default/Dummy)
└───────────────┘
      │
      ▼
┌───────────────┐
│   SDL_Texture │ → Render loop → ImGui Panels
└───────────────┘
```

---
