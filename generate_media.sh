#!/bin/bash
# ----------------------------------------------------------
# Creates images and videos (playfield + backglass) for ASAPCabinetFE
# Opens all tables and screenshots playfield and backglass
# Saves them in table_name/images|video/ folder as:
# table.png|mp4 and backglass.png|mp4 (as set in config.ini)
# ----------------------------------------------------------
# Dependencies: xdotool, ImageMagick, ffmpeg
# Author: Tarso Galvão, Feb/2025

#set -x

# ANSI color codes for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
NC="\033[0m"  # No Color

# INI file
CONFIG_FILE="config.ini"

# Function to get values from INI file
get_ini_value() {
    local section="$1"
    local key="$2"
    local value

    value=$(awk -F= -v section="$section" -v key="$key" '
        BEGIN { inside_section=0 }
        /^\[.*\]$/ { inside_section=($0 == "[" section "]") }
        inside_section && $1 ~ "^[ \t]*" key "[ \t]*$" { gsub(/^[ \t]+|[ \t]+$/, "", $2); gsub(/\r/, "", $2); print $2; exit }
    ' "$CONFIG_FILE")

    echo -e "$value"
}

# Load values from config.ini
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    VPX_EXECUTABLE=$(get_ini_value "VPX" "ExecutableCmd")
    BACKGLASS_VIDEO=$(get_ini_value "CustomMedia" "BackglassVideo")
    TABLE_VIDEO=$(get_ini_value "CustomMedia" "TableVideo")
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "TableImage")
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    echo -e "${GREEN}Loaded config.ini${NC}"
else
    echo -e "${RED}-------------------------------------------------------------${NC}"
    echo -e "${RED}ERROR: config.ini not found. Exiting...${NC}"
    echo -e "${RED}-------------------------------------------------------------${NC}"
    exit 1
fi

# ---------------------------------------------------------------------------
# Configuration variables
# ---------------------------------------------------------------------------
LOAD_DELAY=12       # Seconds to wait after launching VPX for tables to load

# Screenshot-based MP4 video settings:
SCREENSHOT_FPS=12         # Set frames per second here
RECORDING_DURATION=4      # Seconds of video recording

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

    if [ "$NO_VIDEO" == "true" ]; then
        FRAME_COUNT=1
        FRAME_INTERVAL=0
    else
        mkdir -p "$VIDEO_DIR"
    fi

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

        # Save the middle frame with the same base name as OUTPUT_FILE
        if [[ $i -eq $((FRAME_COUNT / 2)) || "$NO_VIDEO" == "true" ]]; then
            local BASE_NAME
            BASE_NAME=$(basename "$OUTPUT_FILE" .mp4)
            local IMAGES_DIR
            IMAGES_DIR=$(dirname "$TABLE_IMAGE")
            local DEST_FILE="${TABLE_DIR}/${IMAGES_DIR}/${BASE_NAME}.png"

            # Skip if file exists and FORCE is "false"
            if [[ -f "$DEST_FILE" && "$FORCE" == "false" ]]; then
                echo -e "Skipping image capture:${YELLOW} ${DEST_FILE}${NC} already exists."
                continue
            fi

            mkdir -p "$TABLE_DIR/$IMAGES_DIR"  # Ensure the directory exists
            cp "$FRAME_FILE" "$DEST_FILE"
            
            if [[ $? -ne 0 ]]; then
                echo -e "${RED}Error: Failed to copy frame to ${DEST_FILE}${NC}" >&2
            else
                echo -e "Frame saved to ${GREEN}${DEST_FILE}${NC}"
            fi
        fi

    done

    if [ "$NO_VIDEO" == "false" ]; then
        echo -e "${YELLOW}Assembling video...${NC}"
        # Assemble the screenshots into an MP4 video using ffmpeg
        ffmpeg -y -framerate "$SCREENSHOT_FPS" -i "${TMP_DIR}/frame_%d.png" \
        -vf "tmix=frames=3:weights=1 1 1" -r 30 -c:v libx264 -pix_fmt yuv420p \
        "$OUTPUT_FILE" 2>&1 | sed -u 's/\r/\n/g' | tail -n 1
    fi

    # Remove the temporary directory and its contents
    rm -rf "$TMP_DIR"
    echo -e "Done."
}

# -----------------------------------------------------------------------------
# Usage function
# -----------------------------------------------------------------------------
usage() {
    echo -e "\nCreates ${GREEN}MP4 videos and PNG images ${YELLOW}(playfield + backglass)${NC} for \033[4mASAPCabinetFE\033[0m"
    echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} following ${YELLOW}config.ini${NC} settings"
    echo -e "\n${BLUE}Usage:${NC} $0 [${BLUE}--missing${NC}|${BLUE}-m${NC}] [${YELLOW}--tables-only${NC}|${YELLOW}-t${NC} [<table_path>] | ${YELLOW}--backglass-only${NC}|${YELLOW}-b${NC} [<table_path>]] [${GREEN}--image-only${NC}|${GREEN}-i${NC}] [${RED}--force${NC}|${RED}-f${NC}]"
    echo ""
    echo -e "  ${BLUE}--missing, -m            Capture missing table and backglass media"
    echo -e "  ${YELLOW}--tables-only, -t        Capture only missing table videos. Optionally provide a specific table path"
    echo -e "  ${YELLOW}--backglass-only, -b     Capture only missing backglass videos. Optionally provide a specific table path"
    echo -e "  ${GREEN}--image-only, -i         Capture only images (skip videos). Optionally provide a specific table path"
    echo -e "  ${RED}--force, -f              Force rebuilding media even if they already exist"
    echo -e "\n  ${NC}-h, --help               Show this help message and exit"
    echo -e "\n${YELLOW}Note:${NC} You can combine args"
    exit 1
}

# -----------------------------------------------------------------------------
# Parse command-line arguments
# -----------------------------------------------------------------------------
MODE=""
SPECIFIC_PATH=""
FORCE="false"
NO_VIDEO="false"

if [ "$#" -eq 0 ]; then
    usage
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        --help|-h)
            usage
            ;;
        --missing|-m)
            MODE="now"
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
        --image-only|-i)
            NO_VIDEO="true"
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
    IMAGES_DIR=$(dirname "$TABLE_IMAGE")
    VIDEO_DIR="${TABLE_DIR}/${TABLE_VIDEO%/*}"
    TABLE_MEDIA_FILE="${TABLE_DIR}/${TABLE_VIDEO}"
    BACKGLASS_MEDIA_FILE="${TABLE_DIR}/${BACKGLASS_VIDEO}"
    TABLE_IMAGE_FILE="${TABLE_DIR}/${TABLE_IMAGE}"
    BACKGLASS_IMAGE_FILE="${TABLE_DIR}/${BACKGLASS_IMAGE}"

    echo -e "${BLUE}Processing: $(basename "$TABLE_DIR")${NC}"

    # Skip processing if media exists (unless forcing rebuild)
    if [ "$NO_VIDEO" == "false" ]; then
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
    else
        if [[ "$MODE" == "now" && -f "$TABLE_IMAGE_FILE" && -f "$BACKGLASS_IMAGE_FILE" && "$FORCE" != "true" ]]; then
            echo -e "${YELLOW}Both PNG files already exist for $(basename "$TABLE_DIR"), skipping.${NC}"
            continue
        fi
        if [[ "$MODE" == "tables-only" && -f "$TABLE_IMAGE_FILE" && "$FORCE" != "true" ]]; then
            echo -e "${YELLOW}Table PNG file already exists for $(basename "$TABLE_DIR"), skipping.${NC}"
            continue
        fi
        if [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_IMAGE_FILE" && "$FORCE" != "true" ]]; then
            echo -e "${YELLOW}Backglass PNG file already exists for $(basename "$TABLE_DIR"), skipping.${NC}"
            continue
        fi
    fi

    # Define a log file to capture VPX output for error checking
    LOG_FILE="/tmp/vpx_log_$(date +%s).txt"

    # Launch VPX in its own process group so that we can later terminate it
    echo -e "${YELLOW}Launching VPX for $(basename "$TABLE_DIR")${NC}"
    setsid "$VPX_EXECUTABLE" -play "$VPX_PATH" > "$LOG_FILE" 2>&1 &
    VPX_PID=$!

    # Initial check to ensure VPX starts successfully
    sleep 3  # Wait 3 seconds to give VPX time to initialize
    if ! kill -0 "$VPX_PID" 2>/dev/null; then
        echo -e "${RED}Error: VPX failed to start or crashed immediately.${NC}"
        exit 1
    fi

    # Wait for VPX windows to load
    echo -e "${GREEN}Waiting $LOAD_DELAY seconds for table to load...${NC}"
    sleep "$LOAD_DELAY"
    echo -e "${YELLOW}Starting screen capture...${NC}"

    # Array to collect capture process IDs
    capture_pids=()

    # Capture table window if needed
    if [[ "$MODE" == "now" || "$MODE" == "tables-only" ]]; then
        WINDOW_ID_VPX=$(xdotool search --name "$WINDOW_TITLE_VPX" | head -n 1)
        if [ -n "$WINDOW_ID_VPX" ]; then
            capture_window_to_mp4 "$WINDOW_ID_VPX" "$TABLE_MEDIA_FILE" &
            capture_pids+=($!)
            if [ "$NO_VIDEO" == "false" ]; then
                echo -e "Will save table MP4 video to ${GREEN}$TABLE_MEDIA_FILE${NC}"
            fi
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
            if [ "$NO_VIDEO" == "false" ]; then
                echo -e "Will save backglass MP4 video to ${GREEN}$BACKGLASS_MEDIA_FILE${NC}"
            fi
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

    # Wait for VPX to complete and capture its exit status
    wait "$VPX_PID"
    EXIT_STATUS=$?

    # Check the exit status and log file for errors
    if [ "$EXIT_STATUS" -ne 0 ]; then
        echo -e "${RED}Error: VPX exited with error code $EXIT_STATUS.${NC}"
    elif grep -q "Error" "$LOG_FILE" 2>/dev/null; then
        echo -e "${RED}Error: VPX encountered an issue according to the log.${NC}"
    else
        echo -e "${GREEN}VPX exited normally.${NC}"
        rm "$LOG_FILE"
    fi

    echo -e "${GREEN}Finished processing table: $TABLE_NAME${NC}"
done 3< <(echo "$VPX_LIST")

echo -e "${GREEN}Finished processing all media.${NC}"
