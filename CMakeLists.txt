cmake_minimum_required(VERSION 3.10)
project(ASAPCabinetFE VERSION 2.0.0 LANGUAGES CXX)

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
    src/capture/screenshot_manager.cpp
    src/config/config_loader.cpp
    src/config/config_gui.cpp
    src/input/input_manager.cpp
    src/table/asset_manager.cpp
    src/table/table_manager.cpp
    src/render/renderer.cpp
    src/render/transition_manager.cpp
    src/render/video_player.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/backends/imgui_impl_sdl2.cpp
    src/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/capture
    src/config
    src/input
    src/table
    src/render
    src/utils
    src/imgui
    src/imgui/backends
    ${CMAKE_BINARY_DIR}  # Where version.h is generated
)

# Find and link dependencies
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(LIBVLC REQUIRED libvlc)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${LIBVLC_LIBRARIES}
)

# Add include directories for pkg-config libraries
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${LIBVLC_INCLUDE_DIRS}
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -D_REENTRANT
)

# Build type-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_LOGGING)
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