// screenshot_capture.cpp
// Refactored to avoid command injection and to detect/use environment-specific screenshot tools.

#include "capture/screenshot_capture.h"
#include "log/logging.h"
#include "utils/os_utils.h"

#include <SDL2/SDL.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace {

// Run command and return whether it exited with status 0.
// args[0] must be the program name.
bool runCommandNoCapture(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    // Convert vector<string> -> char*[] for execvp
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        LOG_ERROR("fork() failed");
        return false;
    }

    if (pid == 0) {
        // child
        // ensure no file descriptors leaked if needed
        execvp(argv[0], argv.data());
        // execvp only returns on error
        _exit(127);
    }

    // parent: wait for child
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        LOG_ERROR("waitpid() failed");
        return false;
    }
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

// Run command and capture stdout. Returns captured text or nullopt on failure.
std::optional<std::string> runCommandCaptureOutput(const std::vector<std::string>& args) {
    if (args.empty()) return std::nullopt;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        LOG_ERROR("pipe() failed");
        return std::nullopt;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]); close(pipefd[1]);
        LOG_ERROR("fork() failed");
        return std::nullopt;
    }

    if (pid == 0) {
        // child: connect stdout to write-end of pipe
        close(pipefd[0]); // close read end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) _exit(127);
        if (dup2(pipefd[1], STDERR_FILENO) == -1) _exit(127); // capture stderr too if useful
        close(pipefd[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    // parent
    close(pipefd[1]); // close write end

    // read child output
    std::ostringstream out;
    char buf[256];
    ssize_t n;
    // simple blocking read (could be improved with select/poll + timeout)
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        out.write(buf, n);
    }
    close(pipefd[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        LOG_ERROR("waitpid() failed");
        return std::nullopt;
    }

    if (!(WIFEXITED(status))) {
        LOG_WARN("Child did not exit normally");
        return std::nullopt;
    }

    // return stdout (may contain newline)
    std::string result = out.str();
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

// Trim helpers
static inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

} // anonymous namespace

// Implementation

ScreenshotCapture::ScreenshotCapture(const std::string& exeDir) : exeDir_(exeDir) {}

/*
  Strategy:
  - Detect session type: X11 vs Wayland via OSUtils::getSessionType()
  - Detect available tools via OSUtils::hasCommand()
  - For X11: use xdotool to find/activate window, ImageMagick 'import' to capture by window id
  - For Wayland:
      * If Hyprland and hyprshot available -> use hyprshot (or hyprshot -o <file>)
      * Else if grim available -> use grim (window capture with slurp is interactive, may need helper)
      * If no tool available -> fail with a helpful log message
  - All external programs are invoked via execvp variants above.
*/

void ScreenshotCapture::captureAllScreenshots(const std::string& playfieldImage, const std::string& backglassImage,
                                             const std::string& dmdImage, SDL_Window* window) {
    std::vector<std::thread> threads;
    threads.emplace_back([this, playfieldImage]() {
        captureScreenshot("Visual Pinball Player", playfieldImage);
    });

    if (isWindowVisible("B2SBackglass")) {
        threads.emplace_back([this, backglassImage]() {
            captureScreenshot("B2SBackglass", backglassImage);
        });
    } else {
        LOG_WARN("B2SBackglass window not visible.");
    }

    if (isWindowVisible("Backglass")) {
        threads.emplace_back([this, backglassImage]() {
            captureScreenshot("Backglass", backglassImage);
        });
    } else {
        LOG_WARN("Backglass window not visible.");
    }

    std::string dmdWindows[] = {"Score", "FlexDMD", "PinMAME", "B2SDMD", "PUPDMD", "PUPFullDMD"};
    bool dmdCaptured = false;
    for (const auto& dmd : dmdWindows) {
        if (isWindowVisible(dmd)) {
            threads.emplace_back([this, dmd, dmdImage]() {
                captureScreenshot(dmd, dmdImage);
            });
            dmdCaptured = true;
            break;
        }
    }
    if (!dmdCaptured) {
        LOG_WARN("No visible DMD window detected.");
    }

    for (auto& thread : threads) {
        if (thread.joinable()) thread.join();
    }

    SDL_RaiseWindow(window);

    // Safe refocus: use xdotool if available; else skip
    if (OSUtils::hasCommand("xdotool")) {
        runCommandNoCapture({"xdotool", "search", "--name", "VPX Screenshot", "windowactivate"});
    } else {
        LOG_INFO("xdotool not available; skipping VPX Screenshot refocus.");
    }
}

void ScreenshotCapture::captureScreenshot(const std::string& windowName, const std::string& outputPath) {
    auto session = OSUtils::getSessionType(); // "x11" or "wayland" or "unknown"
    auto desktop = OSUtils::getDesktopEnv(); // e.g., "Hyprland", "GNOME", etc.

    LOG_INFO("Attempting screenshot for window '" + windowName + "' on session: " + session + " desktop: " + desktop);

    // Ensure output directory exists
    try {
        fs::path outPath(outputPath);
        if (!outPath.has_parent_path()) {
            LOG_ERROR("Output path has no parent: " + outputPath);
            return;
        }
        fs::create_directories(outPath.parent_path());
    } catch (const std::exception& ex) {
        LOG_ERROR(std::string("Failed to create directory for ") + outputPath + " : " + ex.what());
        return;
    }

    // X11 path (preferred when session == "x11" or xdotool available)
    if (session == "x11" || OSUtils::hasCommand("xdotool")) {
        if (!OSUtils::hasCommand("xdotool")) {
            LOG_ERROR("xdotool is not installed; cannot capture X11 windows safely.");
            return;
        }

        // Get window id (safe capture)
        auto maybeWinId = runCommandCaptureOutput({"xdotool", "search", "--name", windowName});
        if (!maybeWinId) {
            LOG_WARN("xdotool search returned no output for '" + windowName + "'");
            return;
        }
        std::string windowId = trim(*maybeWinId);
        if (windowId.empty()) {
            LOG_WARN("Window '" + windowName + "' not found (empty id).");
            return;
        }

        // Activate and raise (no shell)
        runCommandNoCapture({"xdotool", "windowactivate", windowId});
        runCommandNoCapture({"xdotool", "windowraise", windowId});

        // sleep briefly to allow window to come to front
        usleep(400000);

        // Capture using ImageMagick import if available
        if (OSUtils::hasCommand("import")) {
            bool ok = runCommandNoCapture({"import", "-window", windowId, outputPath});
            if (ok) LOG_INFO("Saved screenshot to " + outputPath);
            else LOG_ERROR("Failed to save screenshot to " + outputPath + " using import.");
            return;
        }

        // fallback: xwd -> convert (ImageMagick) if available
        if (OSUtils::hasCommand("xwd") && OSUtils::hasCommand("convert")) {
            // xwd -silent -id <winid> -out /tmp/tmp.xwd && convert /tmp/tmp.xwd <output>
            std::string tmp = outputPath + ".xwd.tmp";
            if (runCommandNoCapture({"xwd", "-id", windowId, "-out", tmp})) {
                if (runCommandNoCapture({"convert", tmp, outputPath})) {
                    LOG_INFO("Saved screenshot to " + outputPath);
                    // try to remove temp file
                    runCommandNoCapture({"rm", "-f", tmp});
                    return;
                } else {
                    LOG_ERROR("convert failed to produce " + outputPath);
                }
            } else {
                LOG_ERROR("xwd failed for window " + windowId);
            }
            // try to cleanup
            runCommandNoCapture({"rm", "-f", tmp});
            return;
        }

        LOG_ERROR("No X11 screenshot tool found (import or xwd+convert).");
        return;
    }

    // Wayland path
    if (session == "wayland") {
        // Hyprland-specific path
        bool hyprshotAvailable = OSUtils::hasCommand("hyprshot") || OSUtils::hasCommand("hyprshotctl");
        bool grimAvailable = OSUtils::hasCommand("grim");
        //bool wlrootsGrabAvailable = OSUtils::hasCommand("wlr-randr"); // example; not used directly

        // Path A: try hyprshot if available (non-interactive, single-shot)
        if (hyprshotAvailable) {
            // Hyprshot usually supports: hyprshot -o <file> or hyprshot <file>
            // Try first variant; if it fails try the other.
            if (OSUtils::hasCommand("hyprshot")) {
                if (runCommandNoCapture({"hyprshot", "-o", outputPath})) {
                    LOG_INFO("Saved screenshot to " + outputPath + " via hyprshot -o");
                    return;
                }
                if (runCommandNoCapture({"hyprshot", outputPath})) {
                    LOG_INFO("Saved screenshot to " + outputPath + " via hyprshot <file>");
                    return;
                }
            }
            if (OSUtils::hasCommand("hyprshotctl")) {
                if (runCommandNoCapture({"hyprshotctl", "screenshot", outputPath})) {
                    LOG_INFO("Saved screenshot to " + outputPath + " via hyprshotctl");
                    return;
                }
            }
        }

        // Path B: try grim (common on wlroots). Grim often requires a region or a window id via slurp.
        if (grimAvailable && OSUtils::hasCommand("slurp")) {
            // This usually opens a selection UI for region; not desired for automated capture.
            // If your compositor supports capturing active window directly via grim, call that.
            // We'll try to use grim full-screen if window selection isn't possible.
            if (runCommandNoCapture({"grim", outputPath})) {
                LOG_INFO("Saved fullscreen Wayland screenshot to " + outputPath + " via grim");
                return;
            }
        }

        LOG_ERROR("No supported Wayland screenshot tool found (hyprshot or grim+slurp).");
        return;
    }

    // Unknown session: attempt best-effort with xdotool/import if available
    if (OSUtils::hasCommand("xdotool") && OSUtils::hasCommand("import")) {
        auto maybeWinId = runCommandCaptureOutput({"xdotool", "search", "--name", windowName});
        if (maybeWinId) {
            std::string windowId = trim(*maybeWinId);
            runCommandNoCapture({"xdotool", "windowactivate", windowId});
            runCommandNoCapture({"xdotool", "windowraise", windowId});
            usleep(400000);
            if (runCommandNoCapture({"import", "-window", windowId, outputPath})) {
                LOG_INFO("Saved screenshot to " + outputPath);
                return;
            }
        }
    }

    LOG_ERROR("Unable to determine a safe capture method for this environment.");
}

bool ScreenshotCapture::isWindowVisible(const std::string& title) {
    // Use xdotool search --name <title> if available
    if (OSUtils::hasCommand("xdotool")) {
        auto maybeOut = runCommandCaptureOutput({"xdotool", "search", "--name", title});
        bool visible = (maybeOut.has_value() && !trim(*maybeOut).empty());
        LOG_INFO("X11/Wayland check for '" + title + "': " + (visible ? "visible" : "not visible"));
        return visible;
    }

    // Wayland or unknown - we cannot reliably determine window visibility without compositor-specific APIs.
    LOG_INFO("Cannot reliably check window visibility for '" + title + "' (xdotool missing). Assuming not visible.");
    return false;
}
