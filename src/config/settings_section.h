#ifndef SETTINGS_SECTION_H
#define SETTINGS_SECTION_H

#include <string>
#include <vector>
#include <unordered_map>

struct SettingsSection {
    std::vector<std::pair<std::string, std::string>> keyValues;
    std::unordered_map<std::string, size_t> keyToLineIndex;
};

#endif // SETTINGS_SECTION_H