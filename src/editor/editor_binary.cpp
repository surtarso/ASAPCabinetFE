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

// TODO:
// - add flags to editor binary to open in specific panels
// - e.g.: ASAPCabinetFE-Editor --browser
// --browser|-b (to open vpsdb browser directly)
// --ini|-i (to open ini editor directly in vpinballx.ini)
//
// - add operational flags for automation
// - e.g.: ASAPCabinetFE-Editor --patch-table "/path/to/MyTable.vpx"
// --rescan|-r (to rescan tables with current settings)
// --rescan-table|-rt <tablefile> (to rescan a single table)
// --rescan-refresh|-rr (to force rescan using all scanners)
// --rescan-table-refresh|-rtr <tablefile> (to force rescan of a single table)
// --patch|-p (to apply patches to all tables)
// --patch-table|-pt <tablefile> (to apply patches to a single table)
// --script|-s <tablefile> (to extract vbs script from a single table)
//
// - config/query helper flags
// --list-monitors|-lm (to list available monitor IDs/screen/resolutions)
// --list-unmatched |-lu (to list tables without vpsdb IDs)
