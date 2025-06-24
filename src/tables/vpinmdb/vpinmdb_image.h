/**
 * @file vpinmdb_image.h
 * @brief Defines image manipulation functions for VpinMdb media in ASAPCabinetFE.
 *
 * This header provides functions for resizing and rotating images using FFmpeg, used for table media.
 */

#ifndef VPINMDB_IMAGE_H
#define VPINMDB_IMAGE_H

#include "log/logging.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace vpinmdb {

/**
 * @brief Resizes an image to the specified dimensions if resizeToWindows is true.
 * @param srcPath Source image path.
 * @param width Target width.
 * @param height Target height.
 * @return True if successful or resizing not needed, false otherwise.
 */
bool resizeImage(const fs::path& srcPath, int width, int height);

/**
 * @brief Rotates an image 90 degrees clockwise if shouldRotate is true.
 * @param srcPath Source image path.
 * @param shouldRotate Whether to perform rotation.
 * @return True if successful or rotation not needed, false otherwise.
 */
bool rotateImage(const fs::path& srcPath, bool shouldRotate);

} // namespace vpinmdb

#endif // VPINMDB_IMAGE_H