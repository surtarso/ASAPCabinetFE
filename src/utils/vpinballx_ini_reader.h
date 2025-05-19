#ifndef VPINBALLX_INI_READER_H
#define VPINBALLX_INI_READER_H

#include <string>
#include <optional>

struct VPinballXIniSettings {
    // Playfield settings
    std::optional<int> playfieldX;
    std::optional<int> playfieldY;
    std::optional<int> playfieldWidth;
    std::optional<int> playfieldHeight;

    // Backglass settings
    std::optional<int> backglassX;
    std::optional<int> backglassY;
    std::optional<int> backglassWidth;
    std::optional<int> backglassHeight;

    // DMD settings
    std::optional<int> dmdX;
    std::optional<int> dmdY;
    std::optional<int> dmdWidth;
    std::optional<int> dmdHeight;
};

class VPinballXIniReader {
public:
    explicit VPinballXIniReader(const std::string& iniPath);
    std::optional<VPinballXIniSettings> readIniSettings() const;

private:
    std::string iniPath_;
};

#endif // VPINBALLX_INI_READER_H