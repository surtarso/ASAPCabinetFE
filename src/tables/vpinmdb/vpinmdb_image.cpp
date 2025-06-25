/**
 * @file vpinmdb_image.cpp
 * @brief Implements image manipulation functions for VpinMdb media.
 *
 * This file provides the implementation for resizing and rotating images using FFmpeg.
 */

#include "vpinmdb_image.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace vpinmdb {

bool resizeImage(const fs::path& srcPath, int width, int height) {
    LOG_INFO("resizeImage called for " + srcPath.string() + " to " + std::to_string(width) + "x" + std::to_string(height));
    std::string ffmpegPath = "/usr/bin/ffmpeg";
    std::stringstream cmd;
    fs::path tempPath = srcPath.parent_path() / ("temp_" + srcPath.filename().string());
    fs::path errorLogPath = srcPath.parent_path() / ("ffmpeg_error_" + srcPath.filename().string() + ".log");

    // Adding -noautorotate and explicit format/setsar for robustness in resize step too
    cmd << "\"" << ffmpegPath << "\" -y -loglevel error -noautorotate -i \"" << srcPath.string()
        << "\" -vf \"scale=" << width << ":" << height << ",format=yuv420p,setsar=1\" \"" << tempPath.string()
        << "\" 2>\"" << errorLogPath.string() << "\"";

    LOG_INFO("Executing FFmpeg resize command: " + cmd.str());
    int ret = std::system(cmd.str().c_str());

    // Read FFmpeg error log
    std::string ffmpegError;
    {
        std::ifstream errorFile(errorLogPath);
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            ffmpegError = buffer.str();
            errorFile.close();
            fs::remove(errorLogPath);
        }
    }

    if (ret != 0) {
        LOG_ERROR("FFmpeg resize failed for " + srcPath.string() + ", return code: " + std::to_string(ret));
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " + ffmpegError);
        }
        if (fs::exists(tempPath)) {
            fs::remove(tempPath);
        }
        return false;
    }

    // Verify output file exists
    if (!fs::exists(tempPath)) {
        LOG_ERROR("FFmpeg did not create resized image: " + tempPath.string());
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " + ffmpegError);
        }
        return false;
    }

    // Replace original file with resized file
    try {
        fs::rename(tempPath, srcPath);
        LOG_INFO("Saved resized image to " + srcPath.string() + ", dimensions: " + std::to_string(width) + "x" + std::to_string(height));
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to rename resized image: " + std::string(e.what()));
        fs::remove(tempPath);
        return false;
    }

    return true;
}

bool rotateImage(const fs::path& srcPath, bool shouldRotate) {
    LOG_INFO("Entering rotateImage for " + srcPath.string());
    LOG_INFO("rotateImage received shouldRotate parameter: " + std::to_string(shouldRotate));

    if (!shouldRotate) {
        LOG_INFO("Explicitly skipping rotation for " + srcPath.string() + " as 'shouldRotate' is false.");
        return true;
    }

    LOG_INFO("Proceeding with rotation for playfield image 90 degrees clockwise: " + srcPath.string());

    std::string ffmpegPath = "/usr/bin/ffmpeg";
    fs::path tempPath = srcPath.parent_path() / ("temp_" + srcPath.filename().string());
    fs::path errorLogPath = srcPath.parent_path() / ("ffmpeg_error_" + srcPath.filename().string() + ".log");

    std::stringstream cmd;
    cmd << "\"" << ffmpegPath << "\" -y -loglevel error -noautorotate -i \"" << srcPath.string()
        << "\" -vf \"transpose=1,format=yuv420p,setsar=1\" \"" << tempPath.string() << "\" 2>\"" << errorLogPath.string() << "\"";

    LOG_INFO("Executing FFmpeg command: " + cmd.str());
    int ret = std::system(cmd.str().c_str());

    std::string ffmpegError;
    {
        std::ifstream errorFile(errorLogPath);
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            ffmpegError = buffer.str();
            errorFile.close();
            fs::remove(errorLogPath);
        }
    }

    if (ret != 0) {
        LOG_ERROR("FFmpeg command failed for " + srcPath.string() + ", return code: " + std::to_string(ret));
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " + ffmpegError);
        }
        if (fs::exists(tempPath)) {
            fs::remove(tempPath);
        }
        return false;
    }

    if (!fs::exists(tempPath)) {
        LOG_ERROR("FFmpeg did not create rotated image: " + tempPath.string());
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " + ffmpegError);
        }
        return false;
    }

    try {
        fs::rename(tempPath, srcPath);
        LOG_INFO("Saved rotated image to " + srcPath.string());
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to rename rotated image: " + std::string(e.what()));
        fs::remove(tempPath);
        return false;
    }

    return true;
}

} // namespace vpinmdb