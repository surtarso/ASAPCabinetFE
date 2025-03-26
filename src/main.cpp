#include "core/app.h"
#include "version.h"
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0;
    }

    App app("config.ini");
    int result = app.initialize();
    if (result != 0) { // Exit only on failure
        std::cerr << "Initialization failed with code " << result << std::endl;
        return 1;
    }
    app.run();
    return 0;
}