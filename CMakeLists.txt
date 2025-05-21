cmake_minimum_required(VERSION 3.10)
project(ASAPCabinetFE VERSION 2.1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OpenGL preference to GLVND to use modern OpenGL libraries
set(OpenGL_GL_PREFERENCE GLVND)

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
    src/sound/sound_manager.cpp
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
    src/render/table_loader.cpp
    src/render/renderer.cpp
    src/render/ffmpeg_player.cpp
    src/render/vlc_player.cpp
    src/render/video_player_factory.cpp
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

# Find and link dependencies
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVLC REQUIRED libvlc)
find_package(OpenGL REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
find_package(GLEW REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${LIBVLC_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
    ${LIBVLC_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${GLEW_LIBRARIES}
)

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
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

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