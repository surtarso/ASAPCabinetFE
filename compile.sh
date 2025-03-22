#!/bin/bash

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if the script is running under Bash
if [ -z "$BASH_VERSION" ]; then
    echo "Error: This script must be run with Bash, not sh or dash."
    echo "Please run it as: bash $0"
    exit 1
fi

# Function to check if required dependencies are installed
check_dependencies() {
    local missing=()
    for pkg in "$@"; do
        if ! dpkg -s "$pkg" &> /dev/null; then
            missing+=("$pkg")
        fi
    done
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}Missing dependencies: ${missing[*]}${NC}"
        echo -e "${YELLOW}Please install them using:${NC}"
        echo -e "${YELLOW}sudo apt-get install ${missing[*]}${NC}"
        exit 1
    fi
}

# Function to check if a file exists
check_file_exists() {
    if [ ! -f "$1" ]; then
        echo -e "${RED}File $1 does not exist.${NC}"
        exit 1
    fi
}


# Check all unique dependencies
echo -e "${YELLOW}Checking dependencies${NC}"
check_dependencies build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev xdotool imagemagick

# Check for main source files and imgui directory
echo -e "${YELLOW}Checking source files${NC}"

check_file_exists screenshot_daemon.cpp
if [ ! -d "src/imgui" ]; then
    echo -e "${RED}imgui directory does not exist.${NC}"
    exit 1
fi


# Compile Screenshot Daemon
echo -e "${GREEN}Compiling Screenshot Daemon${NC}"
g++ src/screenshot_daemon.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_ttf -pthread -o screenshot_daemon > logs/compile_screenshot_daemon.log 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}Screenshot Daemon compilation failed. Check compile_screenshot_daemon.log for details.${NC}"
    exit 1
else
    echo -e "${GREEN}Screenshot Daemon compiled successfully.${NC}"
fi

# Success message
echo -e "${GREEN}All components compiled successfully.${NC}"
