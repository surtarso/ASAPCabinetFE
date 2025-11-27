#include "lbdb_image.h"
#include "log/logging.h"
#include <cstdlib>      // for system()
#include <sstream>

namespace lbdb {

bool resizeClearLogo(const fs::path& inputPng, int width, int height) {
    if (!fs::exists(inputPng)) {
        LOG_ERROR("resizeClearLogo: input does not exist → " + inputPng.string());
        return false;
    }

    // Build the ImageMagick command
    // -resize WxH! → force exact dimensions
    // PNG = transparency kept automatically
    std::stringstream cmd;
    cmd << "convert \""
        << inputPng.string()
        << "\" -resize "
        << width << "x" << height << "! "
        << "\""<< inputPng.string() << "\"";

    int result = std::system(cmd.str().c_str());

    if (result != 0) {
        LOG_ERROR("resizeClearLogo: convert failed → " + cmd.str());
        return false;
    }

    return true;
}

}
