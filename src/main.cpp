#include "core/app.h"
#include "version.h" // Include version.h for PROJECT_VERSION
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    // Check for --version flag before creating App
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0; // Exit immediately
    }

    App app;
    app.run();
    return 0;
}