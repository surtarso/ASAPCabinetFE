#ifndef FIRST_RUN_H
#define FIRST_RUN_H

#include "config/iconfig_service.h"
#include "config/ui/config_gui.h"
#include "core/gui_manager.h"
#include <SDL2/SDL.h>

/**
 * @brief Executes the initial configuration using the provided configuration service.
 *
 * This function reads and applies the configuration settings from the specified file path.
 *
 * @param configService Pointer to an IConfigService implementation responsible for configuration management.
 * @param configPath    The file path to the configuration file to be used for initial setup.
 * @return bool       True if the initial configuration was successfully executed, false otherwise.
 */
bool runInitialConfig(IConfigService* configService, const std::string& configPath);

#endif // FIRST_RUN_H