#include <unistd.h>
#include <filesystem>
#include <iostream>

/// @brief Simple launcher that re-executes the main binary with --editor argument.

int main() {
    std::filesystem::path exePath = std::filesystem::read_symlink("/proc/self/exe").parent_path();
    std::string target = (exePath / "ASAPCabinetFE").string();
    const char* args[] = { target.c_str(), "--editor", nullptr };
    execv(target.c_str(), (char* const*)args);

    // If we get here, execv failed
    std::cerr << "ERROR: Tried to run:";
    for (int i = 0; args[i]; ++i)
        std::cerr << " " << args[i];
    std::cerr << std::endl;
    perror("execv");
    std::cerr << "\nFailed to launch editor-mode shortcut.\nNavigate to the main folder and use `./ASAPCabinetFE --editor` instead." << std::endl;
    return 1;
}
