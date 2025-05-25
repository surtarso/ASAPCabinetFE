/**
 * @file config_schema.h
 * @brief Defines the ConfigSchema class for managing configuration variable schemas in ASAPCabinetFE.
 *
 * This header provides the ConfigSchema class, which defines the structure, types, and
 * default values for configuration variables. It supports parsing and setting configuration
 * values in a Settings object, used by DefaultConfigFactory and SettingsParser.
 */

#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include "config/settings.h"
#include "config/settings_section.h"
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <SDL2/SDL.h>

/**
 * @class ConfigSchema
 * @brief Manages the schema for configuration variables.
 *
 * This class defines the structure of configuration variables, including their types,
 * default values, and setters for parsing and applying values to a Settings object.
 * It is used to generate default configurations and validate settings during parsing.
 */
class ConfigSchema {
public:
    /**
     * @brief Enumeration of configuration variable types.
     */
    enum class Type { 
        String,  ///< String type (e.g., file paths, names).
        Int,     ///< Integer type (e.g., window positions, sizes).
        Float,   ///< Float type (e.g., scaling factors).
        Bool,    ///< Boolean type (e.g., enable/disable flags).
        SDLColor ///< SDL_Color type (e.g., UI colors).
    };

    /**
     * @brief Enumeration of post-processing options for configuration values.
     */
    enum class PostProcess { 
        None,           ///< No post-processing.
        DpiScaleFontSize ///< Apply DPI scaling to font sizes.
    };

    /**
     * @struct Variable
     * @brief Represents a configuration variable's metadata.
     *
     * Defines the properties of a configuration variable, including its field in Settings,
     * INI section and key, default value, type, and setters for parsing and default application.
     */
    struct Variable {
        std::string settingsField; ///< Field name in Settings (e.g., "VPXTablesPath").
        std::string iniSection;   ///< INI section name (e.g., "VPX").
        std::string iniKey;       ///< INI key name (e.g., "VPXTablesPath").
        std::variant<std::string, int, float, bool, SDL_Color> defaultValue; ///< Default value of the variable.
        Type type;                ///< Type of the variable (String, Int, etc.).
        bool needsPathResolution; ///< True if the value requires path resolution (e.g., file paths).
        PostProcess postProcess;  ///< Post-processing option (e.g., DPI scaling).
        std::function<void(Settings&, const std::string&)> parseSetter; ///< Function to parse string and set Settings field.
        std::function<void(Settings&, const std::variant<std::string, int, float, bool, SDL_Color>&)> defaultSetter; ///< Function to set default value in Settings.
    };

    /**
     * @brief Constructs a ConfigSchema instance.
     *
     * Initializes the schema with a predefined set of configuration variables.
     */
    ConfigSchema();

    /**
     * @brief Gets the list of configuration variables.
     *
     * @return A const reference to the vector of Variable objects.
     */
    const std::vector<Variable>& getVariables() const { return variables_; }

private:
    std::vector<Variable> variables_; ///< List of configuration variables.

    /**
     * @brief Parses a string value and sets it in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The string value to parse.
     * @param field The Settings field to set (e.g., "VPXTablesPath").
     */
    void parseString(Settings& s, const std::string& val, const std::string& field) const;

    /**
     * @brief Sets a default string value in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The default value variant containing the string.
     * @param field The Settings field to set.
     */
    void defaultString(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;

    /**
     * @brief Parses an integer value and sets it in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The string value to parse as an integer.
     * @param field The Settings field to set.
     */
    void parseInt(Settings& s, const std::string& val, const std::string& field) const;

    /**
     * @brief Sets a default integer value in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The default value variant containing the integer.
     * @param field The Settings field to set.
     */
    void defaultInt(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;

    /**
     * @brief Parses a float value and sets it in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The string value to parse as a float.
     * @param field The Settings field to set.
     */
    void parseFloat(Settings& s, const std::string& val, const std::string& field) const;

    /**
     * @brief Sets a default float value in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The default value variant containing the float.
     * @param field The Settings field to set.
     */
    void defaultFloat(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;

    /**
     * @brief Parses a boolean value and sets it in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The string value to parse as a boolean.
     * @param field The Settings field to set.
     */
    void parseBool(Settings& s, const std::string& val, const std::string& field) const;

    /**
     * @brief Sets a default boolean value in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The default value variant containing the boolean.
     * @param field The Settings field to set.
     */
    void defaultBool(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;

    /**
     * @brief Parses an SDL_Color value and sets it in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The string value to parse as an SDL_Color (e.g., "255,255,255,255").
     * @param field The Settings field to set.
     */
    void parseSDLColor(Settings& s, const std::string& val, const std::string& field) const;

    /**
     * @brief Sets a default SDL_Color value in Settings.
     *
     * @param s The Settings object to modify.
     * @param val The default value variant containing the SDL_Color.
     * @param field The Settings field to set.
     */
    void defaultSDLColor(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;
};

#endif // CONFIG_SCHEMA_H