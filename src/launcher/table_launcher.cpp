/**
 * @file table_launcher.cpp
 * @brief Implementation of the TableLauncher class.
 */

#include "launcher/table_launcher.h"
#include "log/logging.h"
#include <chrono>
#include <string>
#include <cstdlib>
#include <thread>
#include <sys/wait.h>   // for WIFEXITED, WIFSIGNALED, WEXITSTATUS, WTERMSIG
#include <signal.h>     // SIGTERM, SIGINT, SIGHUP, SIGSEGV, etc.

TableLauncher::TableLauncher(IConfigService* configService)
    : configService_(configService) {
    LOG_INFO("TableLauncher Initialized");
}

std::pair<int, float> TableLauncher::launchTable(const TableData& table) {
    LOG_DEBUG("Launching table: " + table.title);

    // Get settings
    const auto& settings = configService_->getSettings();

    // Build command
    std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                         settings.vpxPlayCmd + " \"" + table.vpxFile + "\" " +
                         settings.vpxEndArgs;
    LOG_DEBUG("Command: " + command);

    // Record start time
    auto start = std::chrono::system_clock::now();
    LOG_INFO("Launching " + table.title);

    // Execute command (Linux-specific for now)
    int result = std::system((command + " > /dev/null 2>&1").c_str());

    // Calculate playtime
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> duration = end - start;
    float timePlayed = duration.count();

    // Format playtime as H:M:S
    int totalSeconds = static_cast<int>(timePlayed);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    std::string timeFormatted = std::to_string(hours) + ":" +
                               (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                               (seconds < 10 ? "0" : "") + std::to_string(seconds);
    LOG_INFO("Welcome back to ASAPCabinetFE.");
    LOG_INFO("You Played " + table.title + " for " + timeFormatted);

    return {result, timePlayed};
}


void TableLauncher::launchTableAsync(
    const TableData& table,
    std::function<void(int exitCode, float secondsPlayed)> callback)
{
    std::thread([this, table, callback]() {

        // Run synchronous launcher (blocking) in worker thread
        auto [rawStatus, secondsPlayed] = this->launchTable(table);

        // rawStatus is what std::system() returned; on POSIX this is the "status"
        // returned by waitpid. Decode it properly.
        int mappedExit = -1;       // value we'll pass to callers (0 == success)
        std::string reasonStr;     // human description for logs
        bool treatAsSuccess = false;

        // Log raw status first for debugging
        LOG_DEBUG("TableLauncherAsync: raw status: " + std::to_string(rawStatus));

        // POSIX decode path
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
        if (WIFEXITED(rawStatus)) {
            int exitCode = WEXITSTATUS(rawStatus);
            reasonStr = "exited with code " + std::to_string(exitCode);
            // Treat Wine shutdown "crashes" as normal:
            // 0, 1, -1 (normal)
            // 132 (SIGILL), 134 (SIGABRT), 136 (SIGFPE), 139 (SIGSEGV)
            if (exitCode == 0 || exitCode == 1 || exitCode == -1 ||
                exitCode == 132 || exitCode == 134 ||
                exitCode == 136 || exitCode == 139)
            {
                treatAsSuccess = true;
                mappedExit = 0;
            }
            else {
                mappedExit = exitCode;
                treatAsSuccess = false;
            }
        } else if (WIFSIGNALED(rawStatus)) {
            int sig = WTERMSIG(rawStatus);
            reasonStr = "killed by signal " + std::to_string(sig);

            // Normal user exits
            if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP) {
                treatAsSuccess = true;
                mappedExit = 0;
            }
            // Wine shutdown crashes (harmless)
            else if (sig == SIGSEGV || sig == SIGABRT || sig == SIGILL || sig == SIGFPE) {
                treatAsSuccess = true;
                mappedExit = 0;
                reasonStr += " (Wine cleanup crash ignored)";
            }
            else {
                // Actual fatal signals
                treatAsSuccess = false;
                mappedExit = 128 + sig;
            }
        } else {
            // Unknown encoding; fall back to heuristic
            reasonStr = "unknown wait status";
            mappedExit = rawStatus;
            treatAsSuccess = false;
        }
#else
        // Non-POSIX fallback: try simple heuristics
        if (rawStatus == 0 || rawStatus == 1 || rawStatus == -1) {
            treatAsSuccess = true;
            mappedExit = 0;
            reasonStr = "heuristic: simple success code";
        } else {
            // check high-bit NT/Wine style values
            unsigned int uraw = static_cast<unsigned int>(rawStatus);
            if ((uraw & 0xC0000000u) == 0xC0000000u || (uraw & 0x40000000u) == 0x40000000u) {
                treatAsSuccess = true;
                mappedExit = 0;
                reasonStr = "heuristic: NT-style code (treated success)";
            } else {
                treatAsSuccess = false;
                mappedExit = rawStatus;
                reasonStr = "heuristic: unknown";
            }
        }
#endif

        // Extra Wine/NT heuristic (POSIX branch too): sometimes Wine returns large NT codes
        // encoded into the status integer; allow treating them as success if you want.
        // You can keep or remove this block if it causes false positives.
        {
            unsigned int uraw = static_cast<unsigned int>(rawStatus);
            if (!treatAsSuccess) {
                if ((uraw & 0xC0000000u) == 0xC0000000u || (uraw & 0x40000000u) == 0x40000000u) {
                    // Many Wine/NT status codes are not fatal for table runs — treat as success.
                    LOG_DEBUG("TableLauncherAsync: detected NT/Wine-style status (0x" +
                              std::to_string(uraw) + "), mapping to success.");
                    treatAsSuccess = true;
                    mappedExit = 0;
                    reasonStr += " (NT/Wine heuristic applied)";
                }
            }
        }

        // Log final decision
        if (treatAsSuccess) {
            LOG_DEBUG("TableLauncherAsync: raw status " + std::to_string(rawStatus) +
                      " decoded as [" + reasonStr + "] -> mapped to SUCCESS (0).");
        } else {
            LOG_ERROR("TableLauncherAsync: raw status " + std::to_string(rawStatus) +
                      " decoded as [" + reasonStr + "] -> mapped to FAILURE (" +
                      std::to_string(mappedExit) + ").");
        }

        // Invoke callback with mappedExit and secondsPlayed
        if (callback) {
            callback(mappedExit, secondsPlayed);
        }

    }).detach();
}
