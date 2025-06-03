cmake_minimum_required(VERSION 3.10)
project(ASAPCabinetFE LANGUAGES CXX C)

unset(CMAKE_INSTALL_PREFIX CACHE)

# Set default install prefix to user's home directory if not specified
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/ASAPCabinetFE" CACHE PATH "Installation prefix for ASAPCabinetFE")
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
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate a version.h header with the project version
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# Define source files
set(SOURCES
    src/main.cpp
    src/core/app.cpp
    src/core/dependency_factory.cpp
    src/core/first_run.cpp
    src/core/gui_manager.cpp
    src/core/joystick_manager.cpp
    src/core/playfield_overlay.cpp
    src/core/window_manager.cpp
    src/sound/pulseaudio_player.cpp
    src/capture/process_handler.cpp
    src/capture/screenshot_capture.cpp
    src/capture/screenshot_manager.cpp
    src/capture/screenshot_window.cpp
    src/config/default_config_factory.cpp
    src/config/config_schema.cpp
    src/config/config_service.cpp
    src/config/config_file_handler.cpp
    src/config/settings_parser.cpp
    src/config/ui/button_handler.cpp
    src/config/ui/config_state.cpp
    src/config/ui/config_gui.cpp
    src/config/ui/input_handler.cpp
    src/config/ui/section_renderer.cpp
    src/config/ui/ui_element_renderer.cpp
    src/keybinds/input_manager.cpp
    src/keybinds/keybind_manager.cpp
    src/render/asset_manager.cpp
    src/render/renderer.cpp
    src/render/video_players/video_player_factory.cpp
    src/render/video_players/ffmpeg/ffmpeg_player.cpp
    src/render/video_players/gstreamer/gstreamer_player.cpp
    src/render/video_players/vlc/vlc_player.cpp
    src/tables/table_loader.cpp
    src/tables/vps_database_client.cpp
    src/utils/logger.cpp
    src/utils/vpinballx_ini_reader.cpp
    # Dear ImGui sources
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdlrenderer2.cpp
    # ImGui File Dialog
    ${CMAKE_SOURCE_DIR}/external/ImGuiFileDialog/ImGuiFileDialog.cpp
)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Suppress all warnings for ImGuiFileDialog.cpp
set_source_files_properties(${CMAKE_SOURCE_DIR}/external/ImGuiFileDialog/ImGuiFileDialog.cpp PROPERTIES COMPILE_FLAGS "-w")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/capture
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/config/ui
    ${CMAKE_SOURCE_DIR}/src/keybinds
    ${CMAKE_SOURCE_DIR}/src/render
    ${CMAKE_SOURCE_DIR}/src/render/video_players
    ${CMAKE_SOURCE_DIR}/src/render/video_players/ffmpeg
    ${CMAKE_SOURCE_DIR}/src/render/video_players/gstreamer
    ${CMAKE_SOURCE_DIR}/src/render/video_players/vlc
    ${CMAKE_SOURCE_DIR}/src/sound
    ${CMAKE_SOURCE_DIR}/src/utils
    # Dear ImGui
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    # ImGui File Dialog
    ${CMAKE_SOURCE_DIR}/external/ImGuiFileDialog
    # nlohmann json
    ${CMAKE_SOURCE_DIR}/external/nlohmann/single_include/nlohmann
    ${CMAKE_BINARY_DIR}  # Where version.h is generated
)

# --- Dependencies Start ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_MAIN REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(LIBVLC REQUIRED libvlc)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
find_package(CURL REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${LIBVLC_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_MAIN_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${LIBVLC_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    CURL::libcurl
)
# --- Dependencies End ---

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -D_REENTRANT
)

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
        # config:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/config.ini ${CMAKE_BINARY_DIR}/config.ini
        # tools:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/generate_media.sh ${CMAKE_BINARY_DIR}/generate_media.sh
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/missing_media.sh ${CMAKE_BINARY_DIR}/missing_media.sh
        # logs:
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/logs
    )

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)

    # Install the executable directly into the root of the install prefix
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ./ # Places ASAPCabinetFE directly in ${CMAKE_INSTALL_PREFIX}/
    )

    # Install resource directories directly into the root of the install prefix
    # Assuming assets/img, assets/snd, src/config.ini, and tools/ are relative to CMAKE_SOURCE_DIR
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/img" DESTINATION .)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/snd" DESTINATION .)
    install(FILES "${CMAKE_SOURCE_DIR}/src/config.ini" DESTINATION .)
    install(FILES "${CMAKE_SOURCE_DIR}/tools/generate_media.sh" DESTINATION .)
    install(FILES "${CMAKE_SOURCE_DIR}/tools/missing_media.sh" DESTINATION .)
    endif()

    # Set output directory (this is for the build directory, not install)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
