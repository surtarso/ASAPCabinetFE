#!/bin/bash
# ----------------------------------------------------------
# Creates images (playfield + backglass) for ASAPCabinetFE
# Opens all tables and screenshots playfield and backglass
# Saves them in table_name/images/ folder as:
# table.png and backglass.png
# ----------------------------------------------------------
# Options:
#   --now, -n             Generate both missing playfield and backglass images
#   --tables-only, -t     Capture only missing table images
#   --backglass-only, -b  Capture only missing backglass images
#   --dry-run             Simulate --now mode, printing actions without executing them
#   --wheel               List tables missing wheel.png image and exit
#   --marquee             List tables missing marquee.png image and exit
#   --force, -f           Generate both images, overwriting existing ones
#   --clean, -c           Removes table.png and backglass.png created by this script
#   -h, --help            Show this help message and exit
#
# Dependencies: xdotool, imagemagick
# Tarso Galvão Feb/2025

#set -x

# ANSI color codes
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE='\033[0;34m'
NC="\033[0m" # No Color

CONFIG_FILE="config.ini"

# Function to get values from INI file
get_ini_value() {
    local section="$1"
    local key="$2"
    local value

    echo "Searching for [$section] -> $key in $CONFIG_FILE" >&2  # Debugging output

    value=$(awk -F= -v section="$section" -v key="$key" '
        BEGIN { inside_section=0 }
        /^\[.*\]$/ { inside_section=($0 == "[" section "]") }
        inside_section && $1 ~ "^[ \t]*" key "[ \t]*$" { gsub(/^[ \t]+|[ \t]+$/, "", $2); gsub(/\r/, "", $2); print $2; exit }
    ' "$CONFIG_FILE")

    echo "Found value: '$value'" >&2  # Debugging output
    echo "$value"
}

# Load values from config.ini
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    echo "ROOT_FOLDER: $ROOT_FOLDER"
    
    VPX_EXECUTABLE=$(get_ini_value "VPX" "ExecutableCmd")
    echo "VPX_EXECUTABLE: $VPX_EXECUTABLE"

    WHEEL_IMAGE=$(get_ini_value "CustomMedia" "WheelImage")
    echo "WHEEL_IMAGE: $WHEEL_IMAGE"
    
    DMD_IMAGE=$(get_ini_value "CustomMedia" "DmdImage")
    echo "DMD_IMAGE: $DMD_IMAGE"
    
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    echo "BACKGLASS_IMAGE: $BACKGLASS_IMAGE"
    
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "TableImage")
    echo "TABLE_IMAGE: $TABLE_IMAGE"
else
    echo "ERROR: config.ini not found. Exiting..."
    exit 1
fi

SCREENSHOT_DELAY=12 # seconds
WINDOW_TITLE_VPX="Visual Pinball Player"
WINDOW_TITLE_BACKGLASS="B2SBackglass"

check_xdotool() {
    if ! command -v xdotool &> /dev/null; then
        echo "Error: xdotool is not installed. Please install it to proceed."
        echo "Debian: sudo apt install xdotool"
        exit 1
    fi
}

check_imagemagick() {
    if ! command -v convert &> /dev/null; then
        echo "Error: ImageMagick (convert) is not installed. Please install it to proceed."
        echo "Debian: sudo apt install imagemagick"
        exit 1
    fi
}

# Run checks
check_xdotool
check_imagemagick

# Help function
show_help() {
  echo -e "Create ${GREEN}PNG images ${YELLOW}(playfield + backglass)${NC} for \033[4mASAPCabinetFE\033[0m"
  echo -e "Opens all tables and screenshots playfield and backglass"
  echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} following ${YELLOW}config.ini${NC} settings"
  echo -e "${BLUE}Usage:${NC} $0 [${BLUE}--now${NC} | ${BLUE}--dry-run${NC} | ${BLUE}--tables-only${NC} | ${BLUE}--backglass-only${NC}] [${YELLOW}--wheel${NC}] [${YELLOW}--marquee${NC}] [${RED}--force${NC}] [${RED}--clean${NC}]"
  echo -e ""
  echo -e "  ${BLUE}--now, -n                Capture missing table and backglass images"
  echo -e "  ${BLUE}--tables-only, -t        ${NC}Capture only missing table images"
  echo -e "  ${BLUE}--backglass-only, -b     ${NC}Capture only missing backglass images"
  echo -e "  ${GREEN}--dry-run                Simulate --now mode, printing actions without executing them"
  echo -e "  ${YELLOW}--wheel                  ${NC}List tables missing wheel images and exit"
  echo -e "  ${YELLOW}--marquee                ${NC}List tables missing marquee images and exit"
  echo -e "  ${RED}--force, -f              ${NC}Force rebuilding media even if they already exist"
  echo -e "  ${RED}--clean, -c              Removes all PNG images created by this script"
  echo -e "\n  ${NC}-h, --help               Show this help message and exit"
  echo -e "\n${YELLOW}Note:${NC} No args shows this help."
  exit 0
}

# Check for help or no arguments
if [ "$#" -eq 0 ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    show_help
fi

DRY_RUN=false
FORCE=false
MODE="none" # Default: no action unless specified

# Parse arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        --dry-run)
            DRY_RUN=true
            MODE="both"
            echo -e "${YELLOW}Dry-run mode: No changes will be made.${NC}"
            shift
            ;;
        --force|-f)
            FORCE=true
            MODE="both"
            echo -e "${BLUE}Capturing both images, overwriting existing ones.${NC}"
            shift
            ;;
        --now|-n)
            MODE="both"
            echo -e "${BLUE}Capturing both missing images.${NC}"
            shift
            ;;
        --tables-only|-t)
            MODE="tables"
            echo -e "${BLUE}Capturing only missing table images.${NC}"
            shift
            ;;
        --backglass-only|-b)
            MODE="backglass"
            echo -e "${BLUE}Capturing only missing backglass images.${NC}"
            shift
            ;;
        --wheel)
            echo -e "${YELLOW}This script can't generate wheel images."
            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$WHEEL_IMAGE...${NC}\n"
            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    wheel_file="$table_dir/$WHEEL_IMAGE"
                    if [ ! -f "$wheel_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                    fi
                fi
            done
            echo -e "\n${BLUE}These tables have ${RED}no <table_folder>/$WHEEL_IMAGE${BLUE} images. ${GREEN}You need to download them.${NC}"
            echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$WHEEL_IMAGE${NC}"
            exit 0
            ;;
        --marquee)
            echo -e "${YELLOW}This script can't generate marquee images."
            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$DMD_IMAGE...${NC}\n"
            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    marquee_file="$table_dir/$DMD_IMAGE"
                    if [ ! -f "$marquee_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                    fi
                fi
            done
            echo -e "\n${BLUE}These tables have ${RED}no <table_folder>/$DMD_IMAGE${BLUE} images. ${GREEN}You need to download them.${NC}"
            echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$DMD_IMAGE${NC}"
            exit 0
            ;;
        --clean|-c)
            echo -e "${RED}Removing all table and backglass PNG files from all tables...${NC}"
            find "$ROOT_FOLDER" -type f \( -name "$(basename "$TABLE_IMAGE")" -o -name "$(basename "$BACKGLASS_IMAGE")" \) -exec rm -v {} \;
            echo -e "${GREEN}Clean operation completed.${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            ;;
    esac
done

# If no mode was set, show help
if [ "$MODE" == "none" ]; then
    show_help
fi

find "$ROOT_FOLDER" -name "*.vpx" | while read -r VPX_PATH; do
    TABLE_NAME=$(basename "$VPX_PATH" .vpx)
    TABLE_DIR=$(dirname "$VPX_PATH")
    IMAGES_FOLDER="${TABLE_DIR}/${TABLE_IMAGE%/*}"
    TABLE_SCREENSHOT="${TABLE_DIR}/${TABLE_IMAGE}"
    BACKGLASS_SCREENSHOT="${TABLE_DIR}/${BACKGLASS_IMAGE}"

    echo -e "${BLUE}Processing: $(basename "$(dirname "$VPX_PATH")")${NC}"

    # Skip if images exist and --force isn't used
    if [ "$MODE" == "both" ] && [ -f "$TABLE_SCREENSHOT" ] && [ -f "$BACKGLASS_SCREENSHOT" ] && [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    Both images already exist, skipping.${NC}"
        continue
    elif [ "$MODE" == "tables" ] && [ -f "$TABLE_SCREENSHOT" ] && [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    Table image already exists, skipping.${NC}"
        continue
    elif [ "$MODE" == "backglass" ] && [ -f "$BACKGLASS_SCREENSHOT" ] && [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    Backglass image already exists, skipping.${NC}"
        continue
    fi

    if [ "$FORCE" == true ]; then
        echo -e "${RED}    --force used, overriding existing files.${NC}"
    fi

    if [ "$DRY_RUN" == true ]; then
        echo -e "${GREEN}    Would create screenshots for: $TABLE_NAME${NC}"
        continue
    fi

    mkdir -p "$IMAGES_FOLDER"

    # Launch VPinballX_GL
    WINE_X11_DRV=x11 "$VPX_EXECUTABLE" -play "$VPX_PATH" > /dev/null 2>&1 &
    VPX_PID=$!

    sleep "$SCREENSHOT_DELAY"

    # Capture table screenshot
    if [ "$MODE" != "backglass" ] && { [ ! -f "$TABLE_SCREENSHOT" ] || [ "$FORCE" == true ]; }; then
        WINDOW_ID_VPX=$(xdotool search --name "$WINDOW_TITLE_VPX" | head -n 1)
        if [ -n "$WINDOW_ID_VPX" ]; then
            if ! import -window "$WINDOW_ID_VPX" "$TABLE_SCREENSHOT"; then
                echo -e "${RED}Error: Failed to capture table screenshot.${NC}"
            else
                echo -e "${GREEN}    Saved table screenshot: $TABLE_SCREENSHOT${NC}"
                mogrify -strip "$TABLE_SCREENSHOT"
            fi
        else
            echo -e "${RED}Error: VPinballX_GL window not found.${NC}"
        fi
    elif [ "$MODE" != "backglass" ]; then
        echo -e "${YELLOW}    Table screenshot already exists, skipping capture.${NC}"
    fi

    # Capture backglass screenshot
    if [ "$MODE" != "tables" ] && { [ ! -f "$BACKGLASS_SCREENSHOT" ] || [ "$FORCE" == true ]; }; then
        WINDOW_ID_BACKGLASS=$(xdotool search --name "$WINDOW_TITLE_BACKGLASS" | head -n 1)
        if [ -n "$WINDOW_ID_BACKGLASS" ]; then
            if ! import -window "$WINDOW_ID_BACKGLASS" "$BACKGLASS_SCREENSHOT"; then
                echo -e "${RED}Error: Failed to capture backglass screenshot.${NC}"
            else
                echo -e "${GREEN}    Saved backglass screenshot: $BACKGLASS_SCREENSHOT${NC}"
                mogrify -strip "$BACKGLASS_SCREENSHOT"
            fi
        else
            echo -e "${RED}Error: B2SBackglass window not found.${NC}"
        fi
    elif [ "$MODE" != "tables" ]; then
        echo -e "${YELLOW}    Backglass screenshot already exists, skipping capture.${NC}"
    fi

    # Kill VPX process
    kill "$VPX_PID"
    sleep 2
    if kill -0 "$VPX_PID" 2>/dev/null; then
        kill -9 "$VPX_PID"
    fi

    echo -e "${GREEN}Screenshots saved for $TABLE_NAME${NC}"
done

echo -e "${GREEN}Finished processing all tables.${NC}"