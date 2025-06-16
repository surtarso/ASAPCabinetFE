/**
 * @file first_run.h
 * @brief Declares the runInitialConfig function for initial configuration in ASAPCabinetFE.
 *
 * This header provides the runInitialConfig function, which executes the initial
 * configuration setup using a configuration service and a specified configuration
 * file path. It integrates with GuiManager and ConfigUI to render the configuration
 * UI during the first run of the application.
 */

#ifndef FIRST_RUN_H
#define FIRST_RUN_H

#include "config/iconfig_service.h"
#include "config/ui/config_ui.h"
#include "core/gui_manager.h"
#include <SDL2/SDL.h>

/**
 * @brief Executes the initial configuration setup.
 *
 * This function initializes and runs the configuration UI using the provided
 * configuration service and configuration file path. It sets up an SDL window
 * and renderer, uses GuiManager for ImGui rendering, and applies settings via
 * ConfigUI.
 *
 * @param configService The configuration service for managing settings.
 * @param configPath The file path to the configuration file.
 * @return True if the configuration was successfully applied, false otherwise.
 */
bool runInitialConfig(IConfigService* configService, IKeybindProvider* keybindProvider, const std::string& configPath);

#endif // FIRST_RUN_H