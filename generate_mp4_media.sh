#!/bin/bash
# ----------------------------------------------------------
# Creates images (playfield + backglass) for ASAPCabinetFE
# Opens all tables and screenshots playfield and backglass
# Saves them in table_name/images/ folder as:
# table.png and backglass.png (as set in config.ini)
# ----------------------------------------------------------
# Options:
#   --now                  Process both windows (default behavior if --now is used)
#                          (Only create media for missing files unless --force is used)
#   --tables-only, -t      Capture only the table window media.
#                          Optionally provide a specific table path.
#   --backglass-only, -b   Capture only the backglass window media.
#                          Optionally provide a specific table path.
#   --dry-run              Simulate --now mode, printing actions without executing them
#   --tables               List tables missing 'table.mp4' videos and exit.
#   --backglass            List tables missing 'backglass.mp4' videos and exit.
#   --dmd                  List tables missing 'dmd.mp4' videos and exit.
#   --force                Force rebuilding media even if they already exist.
#   --clean                Removes all MP4 videos (created by this script).
#   -h, --help             Show this help message and exit
#
# Dependencies: xdotool, ImageMagick, ffmpeg
# Author: Tarso Galvão, Feb/2025

#set -x

# ANSI color codes for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
NC="\033[0m"  # No Color

# Function to get values from INI file
CONFIG_FILE="config.ini"

# Function to get values from INI file
echo -e "${BLUE}Initializing variables...${NC}"
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

    echo -e "Found value: '$value'" >&2  # Debugging output
    echo -e "$value"
}

# Load values from config.ini
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    echo -e "${YELLOW}ROOT_FOLDER: $ROOT_FOLDER${NC}"
    
    VPX_EXECUTABLE=$(get_ini_value "VPX" "ExecutableCmd")
    echo -e "${YELLOW}VPX_EXECUTABLE: $VPX_EXECUTABLE${NC}"
    
    DMD_VIDEO=$(get_ini_value "CustomMedia" "DmdVideo")
    echo -e "${YELLOW}DMD_VIDEO: $DMD_VIDEO${NC}"
    
    BACKGLASS_VIDEO=$(get_ini_value "CustomMedia" "BackglassVideo")
    echo -e "${YELLOW}BACKGLASS_VIDEO: $BACKGLASS_VIDEO${NC}"
    
    TABLE_VIDEO=$(get_ini_value "CustomMedia" "TableVideo")
    echo -e "${YELLOW}TABLE_VIDEO: $TABLE_VIDEO${NC}"
else
    echo -e "${RED}ERROR: config.ini not found. Exiting...${NC}"
    exit 1
fi


# ---------------------------------------------------------------------------
# Configuration variables
# ---------------------------------------------------------------------------

SCREENSHOT_DELAY=12       # Seconds to wait after launching VPX
RECORDING_DURATION=4      # Seconds of recording

# Screenshot-based MP4 settings:
# Adjust SCREENSHOT_FPS as needed (set to 10 frames per second here)
SCREENSHOT_FPS=12
FRAME_COUNT=$((RECORDING_DURATION * SCREENSHOT_FPS))
FRAME_INTERVAL=0.1        # Seconds between screenshots (1/SCREENSHOT_FPS)

# Window titles to capture:
WINDOW_TITLE_VPX="Visual Pinball Player"
WINDOW_TITLE_BACKGLASS="B2SBackglass"

# -----------------------------------------------------------------------------
# Function: capture_window_to_mp4
# Captures a window by taking a series of screenshots and then assembling them
# into an MP4 video.
# Parameters:
#   $1 - Window ID to capture.
#   $2 - Output file for the video.
# -----------------------------------------------------------------------------
capture_window_to_mp4() {
    local WINDOW_ID="$1"
    local OUTPUT_FILE="$2"

    # Get window geometry from xdotool
    local GEOM
    GEOM=$(xdotool getwindowgeometry "$WINDOW_ID")
    # Example GEOM output:
    #   Window 41943048
    #   Position: 384,0 (screen: 0)
    #   Geometry: 1080x1920

    # Parse the window position (replace comma with space)
    local POSITION
    POSITION=$(echo "$GEOM" | grep "Position:" | awk '{print $2}' | tr ',' ' ')
    local X Y
    read -r X Y <<< "$POSITION"

    # Parse the window size
    local SIZE
    SIZE=$(echo "$GEOM" | grep "Geometry:" | awk '{print $2}')
    local WIDTH HEIGHT
    WIDTH=$(echo "$SIZE" | cut -dx -f1)
    HEIGHT=$(echo "$SIZE" | cut -dx -f2)

    echo -e "${BLUE}Capturing window ID $WINDOW_ID at ${WIDTH}x${HEIGHT} from position ${X},${Y}${NC}"

    # Ensure the window is active and raised
    xdotool windowactivate "$WINDOW_ID" > /dev/null 2>&1
    xdotool windowraise "$WINDOW_ID" > /dev/null 2>&1
    sleep 0.5

    # Create a temporary directory for screenshots
    local TMP_DIR
    TMP_DIR=$(mktemp -d --tmpdir frame_tmp_XXXXXX)

    local i
    for (( i=0; i<FRAME_COUNT; i++ )); do
        local FRAME_FILE="${TMP_DIR}/frame_${i}.png"
        # Capture a screenshot of the window
        import -window "$WINDOW_ID" "$FRAME_FILE"
        sleep "$FRAME_INTERVAL"
    done

    # Assemble the screenshots into an MP4 video using ffmpeg
    ffmpeg -y -framerate "$SCREENSHOT_FPS" -i "${TMP_DIR}/frame_%d.png" \
      -c:v libx264 -pix_fmt yuv420p "$OUTPUT_FILE" < /dev/null

    # Remove the temporary directory and its contents
    rm -rf "$TMP_DIR"
}

# -----------------------------------------------------------------------------
# Usage function
# -----------------------------------------------------------------------------
usage() {
    echo -e "\nCreates ${GREEN}MP4 videos ${YELLOW}(playfield + backglass)${NC} for \033[4mASAPCabinetFE\033[0m"
    echo -e "Opens all tables and captures playfield and backglass"
    echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} following ${YELLOW}config.ini${NC} settings"
    echo -e "${BLUE}Usage:${NC} $0 [${BLUE}--now${NC} | ${BLUE}--dry-run${NC} | ${BLUE}--tables-only${NC}|${BLUE}-t${NC} [<table_path>] | ${BLUE}--backglass-only${NC}|${BLUE}-b${NC} [<table_path>]] [${YELLOW}--dmd${NC}] [${RED}--force${NC}] [${RED}--clean${NC}]"
    echo ""
    echo -e "  ${BLUE}--now, -n                Capture missing table and backglass videos"
    echo -e "  ${BLUE}--tables-only, -t${NC}        Capture only missing table videos. Optionally provide a specific table path"
    echo -e "  ${BLUE}--backglass-only, -b${NC}     Capture only missing backglass videos. Optionally provide a specific table path"
    echo -e "  ${GREEN}--dry-run                Simulate --now mode, printing actions without executing them"
    echo -e "  ${YELLOW}--table${NC}                  List tables missing table videos and exit"
    echo -e "  ${YELLOW}--backglass${NC}              List tables missing backglass videos and exit"
    echo -e "  ${YELLOW}--dmd${NC}                    List tables missing DMD videos and exit"
    echo -e "  ${RED}--force, -f${NC}              Force rebuilding media even if they already exist"
    echo -e "  ${RED}--clean, -c              Removes all MP4 videos created by this script"
    echo -e "\n  ${NC}-h, --help               Show this help message and exit"
    echo -e "\n${YELLOW}Note:${NC} No args shows this help."
    exit 1
}

# -----------------------------------------------------------------------------
# Parse command-line arguments
# -----------------------------------------------------------------------------
MODE=""
SPECIFIC_PATH=""
FORCE="false"
DRY_RUN="false"  # New flag for dry-run mode

if [ "$#" -eq 0 ]; then
    usage
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        --help|-h)
            usage
            ;;
        --now|-n)
            MODE="now"
            shift
            ;;
        --dry-run)
            MODE="now"
            DRY_RUN="true"
            shift
            ;;
        --tables-only|-t)
            MODE="tables-only"
            shift
            if [ "$#" -gt 0 ] && [[ "$1" != -* ]]; then
                SPECIFIC_PATH="$1"
                shift
            fi
            ;;
        --backglass-only|-b)
            MODE="backglass-only"
            shift
            if [ "$#" -gt 0 ] && [[ "$1" != -* ]]; then
                SPECIFIC_PATH="$1"
                shift
            fi
            ;;
        --force|-f)
            FORCE="true"
            shift
            ;;
        --clean|-c)
            echo -e "${RED}Removing all table and backglass MP4 files from all tables...${NC}"
            find "$ROOT_FOLDER" -type f \( -name "$(basename "$TABLE_VIDEO")" -o -name "$(basename "$BACKGLASS_VIDEO")" \) -exec rm -v {} \;
            echo -e "${GREEN}Media removed.${NC}"
            exit 0
            ;;
        --dmd|--tables|--backglass)
            case "$1" in
                --dmd) VIDEO_TYPE="$DMD_VIDEO" ;;
                --tables) VIDEO_TYPE="$TABLE_VIDEO" ;;
                --backglass) VIDEO_TYPE="$BACKGLASS_VIDEO" ;;
            esac

            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$VIDEO_TYPE...${NC}\n"

            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    mp4_file="$table_dir/$VIDEO_TYPE"
                    if [ ! -f "$mp4_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                    fi
                fi
            done

            echo -e "\n${BLUE}These tables have ${RED}no <table_folder>/$VIDEO_TYPE${BLUE} videos.${NC}"
            echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$VIDEO_TYPE${NC}"
            exit 0
            ;;
        *)
            echo -e "\n${RED}Unknown option: $1${NC}"
            usage
            ;;
    esac
done

# If no valid mode is set, display help
if [ -z "$MODE" ]; then
    usage
fi

# -----------------------------------------------------------------------------
# Dependency checks: Ensure required commands are available.
# -----------------------------------------------------------------------------
command -v xdotool >/dev/null 2>&1 || {
    echo -e "${RED}Error: xdotool is not installed. Please install it (e.g., sudo apt install xdotool).${NC}"
    exit 1
}

command -v import >/dev/null 2>&1 || {
    echo -e "${RED}Error: ImageMagick (import) is not installed. Please install it (e.g., sudo apt install imagemagick).${NC}"
    exit 1
}

command -v ffmpeg >/dev/null 2>&1 || {
    echo -e "${RED}Error: ffmpeg is not installed. Please install it (e.g., sudo apt install ffmpeg).${NC}"
    exit 1
}

# -----------------------------------------------------------------------------
# Build list of VPX files to process.
# If a specific table path is provided, use it; otherwise process all tables.
# -----------------------------------------------------------------------------
VPX_LIST=""
if [ -n "$SPECIFIC_PATH" ]; then
    if [ -d "$SPECIFIC_PATH" ]; then
        VPX_FILE=$(find "$SPECIFIC_PATH" -maxdepth 1 -type f -name "*.vpx" | head -n 1)
        if [ -z "$VPX_FILE" ]; then
            echo -e "${RED}Error: No .vpx file found in directory $SPECIFIC_PATH${NC}"
            exit 1
        fi
        VPX_LIST="$VPX_FILE"
    elif [ -f "$SPECIFIC_PATH" ]; then
        VPX_LIST="$SPECIFIC_PATH"
    else
        echo -e "${RED}Error: Specified path '$SPECIFIC_PATH' is not valid.${NC}"
        exit 1
    fi
else
    VPX_LIST=$(find "$ROOT_FOLDER" -name "*.vpx")
fi

# -----------------------------------------------------------------------------
# Process each VPX file (each table)
# -----------------------------------------------------------------------------
echo -e "${GREEN}Processing VPX files...${NC}"

while IFS= read -r VPX_PATH <&3; do
    # Derive table name and video folder
    TABLE_NAME=$(basename "$VPX_PATH" .vpx)
    TABLE_DIR=$(dirname "$VPX_PATH")
    VIDEO_FOLDER="${TABLE_DIR}/${TABLE_VIDEO%/*}"
    TABLE_MEDIA_FILE="${TABLE_DIR}/${TABLE_VIDEO}"
    BACKGLASS_MEDIA_FILE="${TABLE_DIR}/${BACKGLASS_VIDEO}"

    echo -e "${BLUE}Processing: $(basename "$TABLE_DIR")${NC}"
    
    if [ "$DRY_RUN" == "true" ]; then
        echo -e "${YELLOW}[DRY RUN] Would create directory: $VIDEO_FOLDER${NC}"
    else
        mkdir -p "$VIDEO_FOLDER"
    fi

    # Skip processing if media exists (unless forcing rebuild)
    if [[ "$MODE" == "now" && -f "$TABLE_MEDIA_FILE" && -f "$BACKGLASS_MEDIA_FILE" && "$FORCE" != "true" ]]; then
        echo -e "${YELLOW}Both MP4 files already exist for $(basename "$TABLE_DIR"), skipping.${NC}"
        continue
    fi
    if [[ "$MODE" == "tables-only" && -f "$TABLE_MEDIA_FILE" && "$FORCE" != "true" ]]; then
        echo -e "${YELLOW}Table MP4 file already exists for $(basename "$TABLE_DIR"), skipping.${NC}"
        continue
    fi
    if [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_MEDIA_FILE" && "$FORCE" != "true" ]]; then
        echo -e "${YELLOW}Backglass MP4 file already exists for $(basename "$TABLE_DIR"), skipping.${NC}"
        continue
    fi

    if [ "$DRY_RUN" == "true" ]; then
        echo -e "${YELLOW}[DRY RUN] Would launch VPX for $(basename "$TABLE_DIR")${NC}"
        echo -e "${YELLOW}[DRY RUN] Would wait $SCREENSHOT_DELAY seconds for windows to load${NC}"
        
        if [[ "$MODE" == "now" ]]; then
            echo -e "${YELLOW}[DRY RUN] Would capture table window to: $TABLE_MEDIA_FILE${NC}"
            echo -e "${YELLOW}[DRY RUN] Would capture backglass window to: $BACKGLASS_MEDIA_FILE${NC}"
        fi
        
        echo -e "${YELLOW}[DRY RUN] Would terminate VPX process${NC}"
        echo -e "${GREEN}[DRY RUN] Finished processing table: $TABLE_NAME${NC}"
        continue
    fi

    # Launch VPX in its own process group so that we can later terminate it
    echo -e "${YELLOW}Launching VPX for $(basename "$TABLE_DIR")${NC}"
    setsid "$VPX_EXECUTABLE" -play "$VPX_PATH" > /dev/null 2>&1 &
    VPX_PID=$!

    # Wait for VPX windows to load
    sleep "$SCREENSHOT_DELAY"

    # Array to collect capture process IDs
    capture_pids=()

    # Capture table window if needed
    if [[ "$MODE" == "now" || "$MODE" == "tables-only" ]]; then
        WINDOW_ID_VPX=$(xdotool search --name "$WINDOW_TITLE_VPX" | head -n 1)
        if [ -n "$WINDOW_ID_VPX" ]; then
            capture_window_to_mp4 "$WINDOW_ID_VPX" "$TABLE_MEDIA_FILE" &
            capture_pids+=($!)
            echo -e "${GREEN}Saved table MP4 video: $TABLE_MEDIA_FILE${NC}"
        else
            echo -e "${RED}Error: '$WINDOW_TITLE_VPX' window not found.${NC}"
        fi
    fi

    # Capture backglass window if needed
    if [[ "$MODE" == "now" || "$MODE" == "backglass-only" ]]; then
        WINDOW_ID_BACKGLASS=$(xdotool search --name "$WINDOW_TITLE_BACKGLASS" | head -n 1)
        if [ -n "$WINDOW_ID_BACKGLASS" ]; then
            capture_window_to_mp4 "$WINDOW_ID_BACKGLASS" "$BACKGLASS_MEDIA_FILE" &
            capture_pids+=($!)
            echo -e "${GREEN}Saved backglass MP4 video: $BACKGLASS_MEDIA_FILE${NC}"
        else
            echo -e "${RED}Error: '$WINDOW_TITLE_BACKGLASS' window not found.${NC}"
        fi
    fi

    # Wait for capture processes to finish
    for pid in "${capture_pids[@]}"; do
        wait "$pid"
    done

    # Terminate the entire VPX process group
    echo -e "${YELLOW}Terminating VPX process group (PGID: $VPX_PID)${NC}"
    kill -TERM -- -"$VPX_PID" 2>/dev/null
    sleep 2
    if kill -0 -- -"$VPX_PID" 2>/dev/null; then
        kill -9 -- -"$VPX_PID" 2>/dev/null
    fi

    echo -e "${GREEN}Finished processing table: $TABLE_NAME${NC}"
done 3< <(echo "$VPX_LIST")

echo -e "${GREEN}Finished processing all VPX files.${NC}"