#include "core/app.h"

int main(int argc, char* argv[]) {
    App app;
    int result = app.initialize(argc, argv);
    if (result == 0) {
        return 0; // Exit if --version was specified
    }
    app.run();
    return 0;
}