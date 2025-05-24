#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include "render/itable_loader.h"
#include <filesystem>

namespace fs = std::filesystem;

class TableLoader : public ITableLoader {
public:
    std::vector<TableData> loadTableList(const Settings& settings) override;
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex;
    std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);
    std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath);
    std::string getMusicPath(const std::string& root, const std::string& musicPath);
};

#endif // TABLE_LOADER_H