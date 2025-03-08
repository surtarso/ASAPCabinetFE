#!/bin/bash
# ----------------------------------------------------------
# Creates images (playfield + backglass) for ASAPCabinetFE
# Opens all tables and screenshots playfield and backglass
# Saves them in table_name/images/ folder as:
# table.png and backglass.png (as set in config.ini)
# ----------------------------------------------------------
# Options:
#   --now, -n             Generate both missing playfield and backglass images
#   --tables-only, -t     Capture only missing table images
#   --backglass-only, -b  Capture only missing backglass images
#   --dry-run             Simulate --now mode, printing actions without executing them
#   --tables              List tables missing 'table.png' image and exit
#   --backglass           List tables missing 'backglass.png' image and exit
#   --wheel               List tables missing 'wheel.png' image and exit
#   --dmd             List tables missing 'dmd.png' image and exit
#   --force, -f           Generate both images, overwriting existing ones
#   --clean, -c           Removes 'table.png' and 'backglass.png' created by this script
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

# Function to get values from INI filef
echo -e "${BLUE}Initializing variables...${NC}"
get_ini_value() {
    local section="$1"
    local key="$2"
    local value

    echo -e "Searching for [$section] -> $key in $CONFIG_FILE" >&2  # Debugging output

    value=$(awk -F= -v section="$section" -v key="$key" '
        BEGIN { inside_section=0 }
        /^\[.*\]$/ { inside_section=($0 == "[" section "]") }
        inside_section && $1 ~ "^[ \t]*" key "[ \t]*$" { gsub(/^[ \t]+|[ \t]+$/, "", $2); gsub(/\r/, "", $2); print $2; exit }
    ' "$CONFIG_FILE")

    echo -e "Found value: '$value'" >&2  # Debugging output
    echo -e "$value"
}

# Load values from config.ini
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    echo -e "${YELLOW}ROOT_FOLDER: $ROOT_FOLDER${NC}"
    
    VPX_EXECUTABLE=$(get_ini_value "VPX" "ExecutableCmd")
    echo -e "${YELLOW}VPX_EXECUTABLE: $VPX_EXECUTABLE${NC}"

    WHEEL_IMAGE=$(get_ini_value "CustomMedia" "WheelImage")
    echo -e "${YELLOW}WHEEL_IMAGE: $WHEEL_IMAGE${NC}"
    
    DMD_IMAGE=$(get_ini_value "CustomMedia" "DmdImage")
    echo -e "${YELLOW}DMD_IMAGE: $DMD_IMAGE${NC}"
    
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    echo -e "${YELLOW}BACKGLASS_IMAGE: $BACKGLASS_IMAGE${NC}"
    
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "TableImage")
    echo -e "${YELLOW}TABLE_IMAGE: $TABLE_IMAGE${NC}"
else
    echo -e "${RED}ERROR: config.ini not found. Exiting...${NC}"
    exit 1
fi

SCREENSHOT_DELAY=12 # seconds
WINDOW_TITLE_VPX="Visual Pinball Player"
WINDOW_TITLE_BACKGLASS="B2SBackglass"
# WINDOW_TITLE_DMD1="PinMAME"
# WINDOW_TITLE_DMD2="FlexDMD"

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
  echo -e "\nCreates ${GREEN}PNG images ${YELLOW}(playfield + backglass)${NC} for \033[4mASAPCabinetFE\033[0m"
  echo -e "Opens all tables and screenshots playfield and backglass"
  echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} following ${YELLOW}config.ini${NC} settings"
  echo -e "${BLUE}Usage:${NC} $0 [${BLUE}--now${NC} | ${BLUE}--dry-run${NC} | ${BLUE}--tables-only${NC} | ${BLUE}--backglass-only${NC}] [${YELLOW}--wheel${NC}] [${YELLOW}--dmd${NC}] [${RED}--force${NC}] [${RED}--clean${NC}]"
  echo -e ""
  echo -e "  ${BLUE}--now, -n                Capture missing table, backglass and dmd images"
  echo -e "  ${BLUE}--tables-only, -t        ${NC}Capture only missing table images"
  echo -e "  ${BLUE}--backglass-only, -b     ${NC}Capture only missing backglass images"
  echo -e "  ${BLUE}--dmd-only, -d           ${NC}Capture only missing dmd images"
  echo -e "  ${GREEN}--dry-run                Simulate --now mode, printing actions without executing them"
  echo -e "  ${YELLOW}--table                  ${NC}List tables missing table images and exit"
  echo -e "  ${YELLOW}--backglass              ${NC}List tables missing backglass images and exit"
  echo -e "  ${YELLOW}--wheel                  ${NC}List tables missing wheel images and exit"
#   echo -e "  ${YELLOW}--dmd                    ${NC}List tables missing dmd images and exit"
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
            MODE="all"
            echo -e "${YELLOW}Dry-run mode: No changes will be made.${NC}"
            shift
            ;;
        --force|-f)
            FORCE=true
            MODE="all"
            echo -e "${BLUE}Capturing all images, overwriting existing ones.${NC}"
            shift
            ;;
        --now|-n)
            MODE="all"
            echo -e "${BLUE}Capturing all missing images.${NC}"
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
        # --dmd-only|-d)
        #     MODE="dmd"
        #     echo -e "${BLUE}Capturing only missing dmd images.${NC}"
        #     shift
        #     ;;    
        --wheel|--dmd|--tables|--backglass)
            case "$1" in
                --wheel) IMAGE_TYPE="$WHEEL_IMAGE" ;;
                --dmd) IMAGE_TYPE="$DMD_IMAGE" ;;
                --tables) IMAGE_TYPE="$TABLE_IMAGE" ;;
                --backglass) IMAGE_TYPE="$BACKGLASS_IMAGE" ;;
            esac

            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$IMAGE_TYPE...${NC}\n"

            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    image_file="$table_dir/$IMAGE_TYPE"
                    if [ ! -f "$image_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                    fi
                fi
            done

            echo -e "\n${BLUE}These tables have ${RED}no <table_folder>/$IMAGE_TYPE${BLUE} images.${NC}"
            echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$IMAGE_TYPE${NC}"
            exit 0
            ;;

        --clean|-c)
            echo -e "${RED}Removing all table and backglass PNG files from all tables...${NC}"
            # Extract filenames, regardless of path
            TABLE_FILENAME=$(basename "$(basename "$TABLE_IMAGE")")
            BACKGLASS_FILENAME=$(basename "$(basename "$BACKGLASS_IMAGE")")

            # Find and remove files
            find "$ROOT_FOLDER" -type f \( -name "$TABLE_FILENAME" -o -name "$BACKGLASS_FILENAME" \) -exec rm -v {} \;

            echo -e "${GREEN}Media removed.${NC}"
            exit 0
            ;;
        *)
            echo -e "\n${RED}Unknown option: $1${NC}"
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
    DMD_SCREENSHOT="${TABLE_DIR}/${DMD_IMAGE}"

    echo -e "${BLUE}Processing: $(basename "$(dirname "$VPX_PATH")")${NC}"

    # Skip if images exist and --force isn't used
    if [ "$MODE" == "all" ] && [ -f "$TABLE_SCREENSHOT" ] && [ -f "$BACKGLASS_SCREENSHOT" ] && [ -f "$DMD_SCREENSHOT" ]&& [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    All images already exist, skipping.${NC}"
        continue
    elif [ "$MODE" == "tables" ] && [ -f "$TABLE_SCREENSHOT" ] && [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    Table image already exists, skipping.${NC}"
        continue
    elif [ "$MODE" == "backglass" ] && [ -f "$BACKGLASS_SCREENSHOT" ] && [ "$FORCE" != true ]; then
        echo -e "${YELLOW}    Backglass image already exists, skipping.${NC}"
        continue
    # elif [ "$MODE" == "dmd" ] && [ -f "$DMD_SCREENSHOT" ] && [ "$FORCE" != true ]; then
    #     echo -e "${YELLOW}    DMD image already exists, skipping.${NC}"
    #     continue
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
    if [ "$MODE" != "backglass" ] && [ "$MODE" != "dmd" ] && { [ ! -f "$TABLE_SCREENSHOT" ] || [ "$FORCE" == true ]; }; then
        WINDOW_ID_TABLE=$(xdotool search --name "$WINDOW_TITLE_VPX" | head -n 1)
        if [ -n "$WINDOW_ID_TABLE" ]; then
            if ! import -window "$WINDOW_ID_TABLE" "$TABLE_SCREENSHOT"; then
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
    if [ "$MODE" != "tables" ] && [ "$MODE" != "dmd" ] && { [ ! -f "$BACKGLASS_SCREENSHOT" ] || [ "$FORCE" == true ]; }; then
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

    # # Capture dmd screenshot
    # if [ "$MODE" != "table" ] && [ "$MODE" != "backglass" ] && { [ ! -f "$DMD_SCREENSHOT" ] || [ "$FORCE" == true ]; }; then
    #     # Check if any DMD window exists
    #     WINDOW_ID_DMD=$(xdotool search --name "$WINDOW_TITLE_DMD1" | head -n 1)
        
    #     if [ -z "$WINDOW_ID_DMD" ]; then
    #         WINDOW_ID_DMD=$(xdotool search --name "$WINDOW_TITLE_DMD2" | head -n 1)
    #     fi

    #     # If no DMD window is found, skip the screenshot process and continue the loop
    #     if [ -z "$WINDOW_ID_DMD" ]; then
    #         echo -e "${YELLOW}    No DMD window found, skipping DMD screenshot.${NC}"
    #     else
    #         # Capture screenshot
    #         if import -window "$WINDOW_ID_DMD" "$DMD_SCREENSHOT"; then
    #             echo -e "${GREEN}    Saved dmd screenshot: $DMD_SCREENSHOT${NC}"
    #             mogrify -strip "$DMD_SCREENSHOT"
    #         else
    #             echo -e "${RED}Error: Failed to capture dmd screenshot.${NC}"
    #         fi
    #     fi
    # elif [ "$MODE" != "tables" ]; then
    #     echo -e "${YELLOW}    Dmd screenshot already exists, skipping capture.${NC}"
    # fi

    # Kill VPX process
    kill "$VPX_PID" 2>/dev/null
    sleep 2
    if kill -0 "$VPX_PID" 2>/dev/null; then
        kill -9 "$VPX_PID" 2>/dev/null
    fi

    echo -e "${GREEN}Screenshots saved for $TABLE_NAME${NC}"
done

echo -e "${GREEN}Finished processing all tables.${NC}"