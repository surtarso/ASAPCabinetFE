#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <string>

class PathUtils {
public:
    static std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);
    static std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath);
    static std::string getMusicPath(const std::string& root, const std::string& musicPath);
};

#endif // PATH_UTILS_H