#ifndef LBDB_IMAGE_H
#define LBDB_IMAGE_H

#include <filesystem>

namespace fs = std::filesystem;

namespace lbdb {

/**
 * Resize a LaunchBox Clear Logo specifically to 128x32 using ImageMagick CLI.
 * This uses the system 'convert' binary installed by CMake.
 * Strict resize, preserves transparency.
 */
bool resizeClearLogo(const fs::path& inputPng, int width = 128, int height = 32);

}

#endif
