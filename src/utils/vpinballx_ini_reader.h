/**
 * @file vpinballx_ini_reader.h
 * @brief Defines the VPinballXIniReader class for reading VPX INI settings in ASAPCabinetFE.
 *
 * This header provides the VPinballXIniReader class, which reads VPX INI files to
 * extract settings such as playfield, backglass, and DMD window positions and sizes.
 * These settings are used to configure screenshot capture or other VPX-related operations.
 */

#ifndef VPINBALLX_INI_READER_H
#define VPINBALLX_INI_READER_H

#include <string>
#include <optional>

/**
 * @struct VPinballXIniSettings
 * @brief Represents VPX INI settings for window positions and sizes.
 *
 * This struct holds optional settings for playfield, backglass, and DMD windows,
 * extracted from a VPX INI file, used for configuring screenshot capture or display.
 */
struct VPinballXIniSettings {
    // Playfield settings
    std::optional<int> playfieldX;      ///< X-coordinate of the playfield window.
    std::optional<int> playfieldY;      ///< Y-coordinate of the playfield window.
    std::optional<int> playfieldWidth;  ///< Width of the playfield window.
    std::optional<int> playfieldHeight; ///< Height of the playfield window.

    // Backglass settings
    std::optional<int> backglassX;      ///< X-coordinate of the backglass window.
    std::optional<int> backglassY;      ///< Y-coordinate of the backglass window.
    std::optional<int> backglassWidth;  ///< Width of the backglass window.
    std::optional<int> backglassHeight; ///< Height of the backglass window.

    // DMD settings
    std::optional<int> dmdX;            ///< X-coordinate of the DMD window.
    std::optional<int> dmdY;            ///< Y-coordinate of the DMD window.
    std::optional<int> dmdWidth;        ///< Width of the DMD window.
    std::optional<int> dmdHeight;       ///< Height of the DMD window.
};

/**
 * @class VPinballXIniReader
 * @brief Reads VPX INI files to extract window settings.
 *
 * This class parses a VPX INI file to retrieve settings for playfield, backglass,
 * and DMD windows, returning them as a VPinballXIniSettings struct. It is used to
 * configure screenshot capture or other VPX-related operations.
 */
class VPinballXIniReader {
public:
    /**
     * @brief Constructs a VPinballXIniReader instance.
     *
     * Initializes the reader with the path to the VPX INI file.
     *
     * @param iniPath The path to the VPX INI file.
     */
    explicit VPinballXIniReader(const std::string& iniPath);

    /**
     * @brief Reads settings from the VPX INI file.
     *
     * Parses the INI file and returns the extracted settings as a VPinballXIniSettings
     * struct, or an empty optional if parsing fails.
     *
     * @return An optional containing the VPinballXIniSettings, or std::nullopt if the file cannot be read.
     */
    std::optional<VPinballXIniSettings> readIniSettings() const;

private:
    std::string iniPath_; ///< Path to the VPX INI file.
};

#endif // VPINBALLX_INI_READER_H