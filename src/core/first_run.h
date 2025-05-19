#ifndef FIRST_RUN_H
#define FIRST_RUN_H

#include "config/iconfig_service.h"
#include "config/ui/config_gui.h"
#include "core/gui_manager.h"
#include <SDL2/SDL.h>

bool runInitialConfig(IConfigService* configService, const std::string& configPath);

#endif // FIRST_RUN_H