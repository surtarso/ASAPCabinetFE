#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include "config/settings.h"
#include "config/settings_section.h"
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <SDL2/SDL.h>

class ConfigSchema {
public:
    enum class Type { String, Int, Float, Bool, SDLColor };
    enum class PostProcess { None, DpiScaleFontSize };

    struct Variable {
        std::string settingsField; // e.g., "VPXTablesPath"
        std::string iniSection;   // e.g., "VPX"
        std::string iniKey;       // e.g., "VPXTablesPath"
        std::variant<std::string, int, float, bool, SDL_Color> defaultValue;
        Type type;
        bool needsPathResolution = false;
        PostProcess postProcess = PostProcess::None;
        std::function<void(Settings&, const std::string&)> parseSetter; // Parse string to set field
        std::function<void(Settings&, const std::variant<std::string, int, float, bool, SDL_Color>&)> defaultSetter; // Set default value
    };

    ConfigSchema();
    const std::vector<Variable>& getVariables() const { return variables_; }

private:
    void parseString(Settings& s, const std::string& val, const std::string& field) const;
    void defaultString(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;
    void parseInt(Settings& s, const std::string& val, const std::string& field) const;
    void defaultInt(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;
    void parseFloat(Settings& s, const std::string& val, const std::string& field) const;
    void defaultFloat(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;
    void parseBool(Settings& s, const std::string& val, const std::string& field) const;
    void defaultBool(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;
    void parseSDLColor(Settings& s, const std::string& val, const std::string& field) const;
    void defaultSDLColor(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const;

    std::vector<Variable> variables_;
};

#endif // CONFIG_SCHEMA_H