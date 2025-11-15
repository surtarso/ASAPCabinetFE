cmake_minimum_required(VERSION 3.10)
project(ASAPCabinetFE LANGUAGES CXX C)

unset(CMAKE_INSTALL_PREFIX CACHE)

# Set default install prefix to user's home directory if not specified
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/${PROJECT_NAME}" CACHE PATH "Installation prefix for ASAPCabinetFE")
endif()

# --- Git Versioning Start ---
# This will print the resolved installation prefix during the configure step.
message(STATUS "ASAPCabinetFE Installation Prefix: ${CMAKE_INSTALL_PREFIX}")

# Find Git executable
find_package(Git QUIET)

# Default version info if Git is not found or no tags exist
set(ASAPCABINETFE_VERSION_MAJOR 0)
set(ASAPCABINETFE_VERSION_MINOR 0)
set(ASAPCABINETFE_VERSION_PATCH 0)
set(ASAPCABINETFE_VERSION_BUILD 0) # Used for commit count since last tag
set(ASAPCABINETFE_VERSION_STRING "0.0.0-unknown")
set(ASAPCABINETFE_GIT_HASH "unknown")
set(ASAPCABINETFE_GIT_BRANCH "unknown")
set(ASAPCABINETFE_GIT_DIRTY "")

if (GIT_FOUND)
    # Get the latest Git tag (e.g., v2.1.0)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_TAG_RESULT
    )

    # Get the number of commits since the last tag (for patch or build number)
    # If no tag is found, this will count all commits.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count ${GIT_TAG}..HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_COMMIT_COUNT_RESULT
    )
    if (GIT_COMMIT_COUNT_RESULT EQUAL 0) # If no tag, count all commits
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    # Get the short commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the current branch name
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Check if the repository is dirty (has uncommitted changes)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
    )
    if (GIT_STATUS)
        set(GIT_DIRTY_FLAG "+dirty")
    else()
        set(GIT_DIRTY_FLAG "")
    endif()

    # Parse SemVer from the Git tag
    # Example: v2.1.0 -> 2, 1, 0
    if (GIT_TAG_RESULT EQUAL 0 AND GIT_TAG MATCHES "v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(ASAPCABINETFE_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(ASAPCABINETFE_VERSION_MINOR ${CMAKE_MATCH_2})
        set(ASAPCABINETFE_VERSION_PATCH ${CMAKE_MATCH_3})
        set(ASAPCABINETFE_VERSION_BUILD ${GIT_COMMIT_COUNT}) # Use commit count as build number
    else()
        # Fallback if no valid SemVer tag is found, or no tags at all.
        # Use a base version and append commit count.
        set(ASAPCABINETFE_VERSION_MAJOR 0)
        set(ASAPCABINETFE_VERSION_MINOR 1)
        set(ASAPCABINETFE_VERSION_PATCH ${GIT_COMMIT_COUNT}) # Or use 0 and make build = commit_count
        set(ASAPCABINETFE_VERSION_BUILD ${GIT_COMMIT_COUNT})
        message(STATUS "No valid SemVer tag found. Using base version and commit count.")
    endif()

    set(ASAPCABINETFE_VERSION_STRING "${ASAPCABINETFE_VERSION_MAJOR}.${ASAPCABINETFE_VERSION_MINOR}.${ASAPCABINETFE_VERSION_PATCH}")
    if (NOT GIT_COMMIT_COUNT STREQUAL "0" AND GIT_TAG_RESULT EQUAL 0) # Only append commit count if we are *not* on a tag
        set(ASAPCABINETFE_VERSION_STRING "${ASAPCABINETFE_VERSION_STRING}-${GIT_COMMIT_COUNT}")
    endif()
    set(ASAPCABINETFE_VERSION_STRING "${ASAPCABINETFE_VERSION_STRING}+${GIT_HASH}${GIT_DIRTY_FLAG}")

    set(ASAPCABINETFE_GIT_HASH "${GIT_HASH}")
    set(ASAPCABINETFE_GIT_BRANCH "${GIT_BRANCH}")
    set(ASAPCABINETFE_GIT_DIRTY "${GIT_DIRTY_FLAG}")

else()
    message(WARNING "Git not found. Version information will be set to defaults.")
endif()
# --- End Git Versioning ---

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate a version.h header with the project version
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# -------------------- Fetch modules ---------------------
include(FetchContent)

# Allow legacy FetchContent_Populate usage for libraries without CMake targets
cmake_policy(SET CMP0169 OLD)

# -------------------- ImGui ---------------------
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_MakeAvailable(imgui)
endif()

# -------------------- ImGuiFileDialog ---------------------
# We won't use FetchContent_MakeAvailable for IGFD because it doesn't provide a clean target.
FetchContent_Declare(
    ImGuiFileDialog
    URL https://github.com/aiekick/ImGuiFileDialog/archive/refs/tags/v0.6.8.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(ImGuiFileDialog)
if(NOT imguifiledialog_POPULATED)
    FetchContent_Populate(ImGuiFileDialog)
endif()

# ------------ Rust Library Integration Start --------------

# 1. Fetch the vpin Rust library
FetchContent_Declare(
    vpin
    URL https://github.com/francisdb/vpin/archive/refs/tags/v0.18.6.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(vpin)
if(NOT vpin_POPULATED)
    FetchContent_MakeAvailable(vpin)
endif()

# 2. Find Rust Toolchain (cargo)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
find_program(RUSTC_EXECUTABLE rustc REQUIRED)

# Set the path to your local Rust FFI wrapper SOURCE directory
set(VPIN_FFI_WRAPPER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/vpin_ffi_wrapper")
set(VPIN_FFI_WRAPPER_LIB_NAME "vpin_ffi_wrapper") # Name from Cargo.toml

# Define the directory where the Rust wrapper's Cargo.toml AND SOURCE will be GENERATED/COPIED
# This will now be inside the _deps folder, alongside other fetched dependencies.
set(VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR "${CMAKE_BINARY_DIR}/_deps/${VPIN_FFI_WRAPPER_LIB_NAME}-build")
file(MAKE_DIRECTORY "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}")

# Calculate the relative path for Cargo.toml.in to point to the fetched vpin crate.
# This path is relative from the *generated* Cargo.toml
# (e.g., build/_deps/vpin_ffi_wrapper-build/Cargo.toml)
# to the fetched vpin source directory (e.g., build/_deps/vpin-src).
file(RELATIVE_PATH VPIN_CRATE_PATH_REL "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}" "${vpin_SOURCE_DIR}")
message(STATUS "Calculated VPIN_CRATE_PATH_REL: ${VPIN_CRATE_PATH_REL}") # For debugging

# Configure Cargo.toml for the Rust FFI wrapper
configure_file(
    "${VPIN_FFI_WRAPPER_SOURCE_DIR}/Cargo.toml.in"
    "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/Cargo.toml"
    @ONLY
)

# Copy the Rust source directory and include directory into the build location within _deps
add_custom_command(
    OUTPUT "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/.copy_stamp" # Use a stamp file
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VPIN_FFI_WRAPPER_SOURCE_DIR}/src"
            "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VPIN_FFI_WRAPPER_SOURCE_DIR}/include"
            "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E touch "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/.copy_stamp" # Create the stamp file
    COMMENT "Copying Rust FFI wrapper source and headers into _deps build directory..."
    # Dependencies: This ensures the copy command runs if source files change
    # List all files that, if changed, should trigger a re-copy
    DEPENDS
        "${VPIN_FFI_WRAPPER_SOURCE_DIR}/Cargo.toml.in"
        "${VPIN_FFI_WRAPPER_SOURCE_DIR}/include/vpin_wrapper.h"
        # You might want to list all .rs files in src/ too, for example:
        "${VPIN_FFI_WRAPPER_SOURCE_DIR}/src/lib.rs"
        # If you have more .rs files, you'd add them here
        # E.g., "${VPIN_FFI_WRAPPER_SOURCE_DIR}/src/another_module.rs"
)


# Set output directory for the Rust library (Cargo's target directory)
set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/_deps/rust_lib_target") # Keep this separate for all Cargo output
file(MAKE_DIRECTORY "${RUST_TARGET_DIR}")

# # Determine Cargo build profile (debug/release)
# string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
# if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
#     set(CARGO_PROFILE_FLAG "") # Default debug build, no extra flag needed
#     set(RUST_LIB_SUBDIR "debug")
# else()
set(CARGO_PROFILE_FLAG "--release")
set(RUST_LIB_SUBDIR "release")
# endif()

# 3. Build the Rust FFI wrapper as a static library
add_custom_command(
    OUTPUT "${RUST_TARGET_DIR}/${RUST_LIB_SUBDIR}/lib${VPIN_FFI_WRAPPER_LIB_NAME}.a"
    COMMAND ${CARGO_EXECUTABLE} build
            --manifest-path "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/Cargo.toml"
            --target-dir "${RUST_TARGET_DIR}"
            ${CARGO_PROFILE_FLAG}
    WORKING_DIRECTORY "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}"
    COMMENT "Building Rust FFI wrapper library..."
    # Depend on the configured Cargo.toml AND the stamp file from the copy command
    DEPENDS "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/Cargo.toml"
            "${VPIN_FFI_WRAPPER_BUILD_IN_DEPS_DIR}/.copy_stamp" # <-- Crucial dependency
)

add_custom_target(build_rust_lib ALL
    DEPENDS "${RUST_TARGET_DIR}/${RUST_LIB_SUBDIR}/lib${VPIN_FFI_WRAPPER_LIB_NAME}.a"
)

# Create an interface library for C++ to link against
add_library(${VPIN_FFI_WRAPPER_LIB_NAME} STATIC IMPORTED)
set_target_properties(${VPIN_FFI_WRAPPER_LIB_NAME} PROPERTIES
    IMPORTED_LOCATION "${RUST_TARGET_DIR}/${RUST_LIB_SUBDIR}/lib${VPIN_FFI_WRAPPER_LIB_NAME}.a"
    INTERFACE_INCLUDE_DIRECTORIES "${VPIN_FFI_WRAPPER_SOURCE_DIR}/include" # Point to your original source include
)

# --- Rust Library Integration End ---

# Define source files
set(SOURCES
    # main/entry
    src/main.cpp
    # core
    src/core/app.cpp
    src/core/dependency_factory.cpp
    src/core/first_run.cpp
    src/core/joystick_manager.cpp
    src/core/window_manager.cpp
    src/core/ui/imgui_manager.cpp
    src/core/ui/loading_screen.cpp
    src/core/ui/metadata_panel.cpp
    src/core/ui/modal_dialog.cpp
    src/core/ui/playfield_overlay.cpp
    # launcher
    src/launcher/table_launcher.cpp
    # sound
    src/sound/pulseaudio_player.cpp
    # capture
    src/capture/process_handler.cpp
    src/capture/screenshot_capture.cpp
    src/capture/screenshot_manager.cpp
    src/capture/screenshot_window.cpp
    # config
    src/config/config_service.cpp
    src/config/ui/config_ui.cpp
    src/config/ui/section_renderer.cpp
    # editor
    # src/editor/editor_binary.cpp # gets imported later in binary creation
    src/editor/footer_actions.cpp
    src/editor/header_actions.cpp
    src/editor/editor.cpp
    src/editor/editor_first_run.cpp
    src/editor/sorting_filters.cpp
    src/editor/ui/editor_ui.cpp
    src/editor/ui/editor_header.cpp
    src/editor/ui/editor_body.cpp
    src/editor/ui/editor_footer.cpp
    # keybinds
    src/keybinds/input_manager.cpp
    src/keybinds/keybind_manager.cpp
    #l log
    src/log/logger.cpp
    src/log/logger_proxy.cpp
    # render
    src/render/renderer.cpp
    src/render/assets/asset_manager.cpp
    src/render/assets/texture_cache.cpp
    src/render/assets/title_renderer.cpp
    src/render/assets/video_player_cache.cpp
    src/render/video_players/video_player_factory.cpp
    src/render/video_players/ffmpeg/ffmpeg_player.cpp
    src/render/video_players/ffmpeg/audio_decoder.cpp
    src/render/video_players/ffmpeg/video_decoder.cpp
    src/render/video_players/vlc/vlc_player.cpp
    # tables
    src/tables/asap_index_manager.cpp
    src/tables/file_scanner.cpp
    src/tables/table_loader.cpp
    src/tables/table_patcher.cpp
    src/tables/vpin_scanner.cpp
    src/tables/vpxtool_scanner.cpp
    src/tables/overrides/table_override_manager.cpp
    src/tables/overrides/table_override_editor.cpp
    src/tables/vpinmdb/vpinmdb_client.cpp
    src/tables/vpinmdb/vpinmdb_downloader.cpp
    src/tables/vpinmdb/vpinmdb_image.cpp
    src/tables/vpsdb/vps_database_scanner.cpp
    src/tables/vpsdb/vps_database_client.cpp
    src/tables/vpsdb/vps_database_loader.cpp
    src/tables/vpsdb/vps_database_updater.cpp
    src/tables/vpsdb/vpsdb_catalog_manager.cpp
    src/tables/vpsdb/vpsdb_catalog_image.cpp
    src/tables/vpsdb/vpsdb_catalog_json.cpp
    src/tables/vpsdb/vpsdb_catalog_table.cpp
    # utils
    src/utils/os_utils.cpp
    src/utils/path_utils.cpp
    src/utils/sha_utils.cpp
    src/utils/string_utils.cpp
    src/utils/vpinballx_ini_reader.cpp
    # Dear ImGui sources added directly from FetchContent path
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp"
    # ImGui File Dialog source
    "${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp"
)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Ensure the Rust library is built before linking the main executable
add_dependencies(${PROJECT_NAME} build_rust_lib)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/ui
    ${CMAKE_SOURCE_DIR}/src/capture
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/config/ui
    ${CMAKE_SOURCE_DIR}/src/editor
    ${CMAKE_SOURCE_DIR}/src/keybinds
    ${CMAKE_SOURCE_DIR}/src/log
    ${CMAKE_SOURCE_DIR}/src/render
    ${CMAKE_SOURCE_DIR}/src/render/assets
    ${CMAKE_SOURCE_DIR}/src/render/video_players
    ${CMAKE_SOURCE_DIR}/src/render/video_players/ffmpeg
    ${CMAKE_SOURCE_DIR}/src/render/video_players/vlc
    ${CMAKE_SOURCE_DIR}/src/tables
    ${CMAKE_SOURCE_DIR}/src/tables/overrides
    ${CMAKE_SOURCE_DIR}/src/tables/vpinmdb
    ${CMAKE_SOURCE_DIR}/src/tables/vpsdb
    ${CMAKE_SOURCE_DIR}/src/sound
    ${CMAKE_SOURCE_DIR}/src/utils
    # Dear ImGui includes
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    # ImGui File Dialog includes
    "${imguifiledialog_SOURCE_DIR}"
    # Rust FFI wrapper include
    "${VPIN_FFI_WRAPPER_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}"  # Where version.h is generated
)

# Suppress warnings for ImGuiFileDialog.cpp and ensure it finds imgui.h
set_source_files_properties("${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp" PROPERTIES
    COMPILE_FLAGS "-w"
)
# Suppress warnings for ImGui sources and headers
file(GLOB_RECURSE IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES COMPILE_FLAGS "-w")

# Treat ImGui include path as a system include (silences header warnings)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${imgui_SOURCE_DIR})

# --- Dependencies Start ---
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)
pkg_check_modules(SDL2_MAIN REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(LIBVLC REQUIRED libvlc)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample)
find_package(CURL REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${LIBVLC_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# support VLC v3.x and v4.x
pkg_get_variable(LIBVLC_VERSION libvlc version)
if(LIBVLC_VERSION VERSION_GREATER_EQUAL "4.0.0")
    add_compile_definitions(HAVE_LIBVLC_V4)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    ${SDL2_MAIN_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${LIBVLC_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${VPIN_FFI_WRAPPER_LIB_NAME}
    CURL::libcurl
)
# --- Dependencies End ---

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wshadow -Wconversion -D_REENTRANT
)

# -----------------------------------------------------------------------------
# Primary launcher binary (ASAPCabinetFE)
# -----------------------------------------------------------------------------
# Build type-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_LOGGING)
    # ASan flags
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    target_link_libraries(${PROJECT_NAME} PRIVATE -fsanitize=address)

    # Copy resources to the build directory after building the executable
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # assets:
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/img ${CMAKE_BINARY_DIR}/img
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/snd ${CMAKE_BINARY_DIR}/snd
        # tools:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/generate_media.sh ${CMAKE_BINARY_DIR}/generate_media.sh
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/missing_media.sh ${CMAKE_BINARY_DIR}/missing_media.sh
        # logs/data:
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/logs
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/data
    )

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)

    # Install the executable directly into the root of the install prefix
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION . # Places ASAPCabinetFE directly in ${CMAKE_INSTALL_PREFIX}/
    )

    # Install resource directories directly into the root of the install prefix
    # Assuming assets/img, assets/snd, src/config.ini, and tools/ are relative to CMAKE_SOURCE_DIR
    install(CODE "file(MAKE_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/logs\")")
    install(CODE "file(MAKE_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/data\")")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/img" DESTINATION .)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/snd" DESTINATION .)
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/tools/generate_media.sh" DESTINATION .)
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/tools/missing_media.sh" DESTINATION .)
endif()

# Set output directory (this is for the build directory, not install)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# -----------------------------------------------------------------------------
# Secondary launcher binary (--editor shortcut)
# -----------------------------------------------------------------------------
# Define and import the editor source file
add_executable(${PROJECT_NAME}-Editor src/editor/editor_binary.cpp)

# Place it in the same build folder as the main binary
set_target_properties(${PROJECT_NAME}-Editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Ensure it builds after the main app so ./ASAPCabinetFE exists
add_dependencies(${PROJECT_NAME}-Editor ${PROJECT_NAME})

# Install it alongside the main binary
install(TARGETS ${PROJECT_NAME}-Editor
    RUNTIME DESTINATION .
)
