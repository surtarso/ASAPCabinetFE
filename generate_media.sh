#!/bin/bash
# ----------------------------------------------------------
# Creates images and videos (playfield + backglass + DMD) for ASAPCabinetFE
# Opens all tables and screenshots playfield, backglass, and visible DMD windows
# Saves them in table_name/images|video/ folder as specified in config.ini
# ----------------------------------------------------------
# Dependencies: xdotool, ImageMagick, ffmpeg, xwininfo
# Author: Tarso Galvão, Feb/2025

# Uncomment the line below for debugging
# set -x

# **Color Codes for Output**
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
NC="\033[0m"  # No Color

# **File Paths**
CONFIG_FILE="config.ini"
LOG_FILE="error.log"

# **Configuration Variables**
LOAD_DELAY=12             # Seconds to wait after launching VPX for tables to load
SCREENSHOT_FPS=12         # Frames per second for video
RECORDING_DURATION=4      # Seconds of video recording
FRAME_COUNT=$((RECORDING_DURATION * SCREENSHOT_FPS))
FRAME_INTERVAL=0.1        # Seconds between screenshots (approx 10 FPS, adjusted by ffmpeg)
WINDOW_TITLE_VPX="Visual Pinball Player"
WINDOW_TITLE_BACKGLASS="B2SBackglass"
WINDOW_TITLE_DMD=("FlexDMD" "PinMAME" "B2SDMD") #for now, in order of preference...

# **Function: get_ini_value**
# Reads a value from the specified section and key in the INI file
get_ini_value() {
    local section="$1"
    local key="$2"
    awk -F= -v section="$section" -v key="$key" '
        BEGIN { inside_section=0 }
        /^\[.*\]$/ { inside_section=($0 == "[" section "]") }
        inside_section && $1 ~ "^[ \t]*" key "[ \t]*$" { gsub(/^[ \t]+|[ \t]+$/, "", $2); gsub(/\r/, "", $2); print $2; exit }
    ' "$CONFIG_FILE"
}

# **Load Configuration from config.ini**
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    VPX_EXECUTABLE=$(get_ini_value "VPX" "ExecutableCmd")
    TABLE_VIDEO=$(get_ini_value "CustomMedia" "TableVideo")
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "TableImage")
    BACKGLASS_VIDEO=$(get_ini_value "CustomMedia" "BackglassVideo")
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    DMD_VIDEO=$(get_ini_value "CustomMedia" "DmdVideo")
    DMD_IMAGE=$(get_ini_value "CustomMedia" "DmdImage")
    echo -e "${GREEN}Loaded config.ini${NC}"
else
    echo -e "${RED}-------------------------------------------------------------${NC}"
    echo -e "${RED}Error: config.ini not found. Exiting...${NC}"
    echo -e "${RED}-------------------------------------------------------------${NC}"
    exit 1
fi

# **Function: capture_vpx_window**
# Captures screenshots of a window and assembles them into a video or saves a single image
capture_vpx_window() {
    local window_id="$1"
    local video_output="$2"
    local image_output="$3"

    # Activate and raise the window
    xdotool windowactivate "$window_id" >/dev/null 2>&1
    xdotool windowraise "$window_id" >/dev/null 2>&1
    sleep 0.5

    if [[ "$NO_VIDEO" == "true" ]]; then
        # Ensure the image directory exists
        mkdir -p "$(dirname "$image_output")"
        # Capture one frame and save to image_output
        import -window "$window_id" "$image_output"
        echo -e "Saved image to ${GREEN}$image_output${NC}"
    else
        # Ensure directories exist
        mkdir -p "$(dirname "$video_output")"
        mkdir -p "$(dirname "$image_output")"

        # Create temporary directory for screenshots
        local tmp_dir
        tmp_dir=$(mktemp -d --tmpdir frame_tmp_XXXXXX)

        # Capture screenshots
        for ((i = 0; i < FRAME_COUNT; i++)); do
            local frame_file="${tmp_dir}/frame_${i}.png"
            import -window "$window_id" "$frame_file"
            sleep "$FRAME_INTERVAL"

            # Save the middle frame as image_output
            if [[ $i -eq $((FRAME_COUNT / 2)) ]]; then
                cp "$frame_file" "$image_output" && echo -e "Frame saved to ${GREEN}$image_output${NC}" || {
                    echo -e "${RED}Error: Failed to copy frame to $image_output${NC}" >&2
                }
            fi
        done

        # Assemble video
        echo -e "${YELLOW}Assembling video...${NC}"
        ffmpeg -y -framerate "$SCREENSHOT_FPS" -i "${tmp_dir}/frame_%d.png" \
            -vf "tmix=frames=3:weights=1 1 1" -r 30 -c:v libx264 -pix_fmt yuv420p \
            "$video_output" 2>&1 | sed -u 's/\r/\n/g' | tail -n 1

        rm -rf "$tmp_dir"
    fi
}

# Function to check if .vbs indicates a DMD
has_dmd_from_vbs() {
    local vbs_file="$1"
    if grep -q -i -E "FlexDMD|B2SDMD|PinMAME|UseDMD|Controller.DMD" "$vbs_file"; then
        return 0  # DMD present
    else
        return 1  # No DMD
    fi
}

# **Function: usage**
# Displays help message
usage() {
    echo -e "\nCreates ${GREEN}MP4 videos and PNG images ${YELLOW}(playfield + backglass + DMD)${NC} for \033[4mASAPCabinetFE\033[0m"
    echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} per ${YELLOW}config.ini${NC}"
    echo -e "\n${BLUE}Usage:${NC} $0 [${BLUE}--missing${NC}|${BLUE}-m${NC}] [${YELLOW}--tables-only${NC}|${YELLOW}-t${NC} [<table_path>] | ${YELLOW}--backglass-only${NC}|${YELLOW}-b${NC} [<table_path>] | ${YELLOW}--dmd-only${NC}|${YELLOW}-d${NC} [<table_path>]] [${GREEN}--image-only${NC}|${GREEN}-i${NC}] [${RED}--force${NC}|${RED}-f${NC}]"
    echo -e "\nOptions:"
    echo -e "  ${BLUE}--missing, -m            Capture missing table, backglass, and DMD media"
    echo -e "  ${YELLOW}--tables-only, -t        Capture only table media (optional path)"
    echo -e "  ${YELLOW}--backglass-only, -b     Capture only backglass media (optional path)"
    echo -e "  ${YELLOW}--dmd-only, -d           Capture only DMD media (optional path)"
    echo -e "  ${GREEN}--image-only, -i         Capture images only (optional path)"
    echo -e "  ${RED}--force, -f              Force rebuild even if files exist"
    echo -e "  ${NC}-h, --help               Show this help"
    echo -e "\n${YELLOW}Note:${NC} Combine options as needed"
    exit 1
}

# **Parse Command-Line Arguments**
MODE=""
SPECIFIC_PATH=""
FORCE="false"
NO_VIDEO="false"

[[ $# -eq 0 ]] && usage

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) usage ;;
        --missing|-m) MODE="now"; shift ;;
        --tables-only|-t)
            MODE="tables-only"
            shift
            [[ $# -gt 0 && "$1" != -* ]] && { SPECIFIC_PATH="$1"; shift; }
            ;;
        --backglass-only|-b)
            MODE="backglass-only"
            shift
            [[ $# -gt 0 && "$1" != -* ]] && { SPECIFIC_PATH="$1"; shift; }
            ;;
        --dmd-only|-d)
            MODE="dmd-only"
            shift
            [[ $# -gt 0 && "$1" != -* ]] && { SPECIFIC_PATH="$1"; shift; }
            ;;
        --image-only|-i)
            NO_VIDEO="true"
            shift
            [[ $# -gt 0 && "$1" != -* ]] && { SPECIFIC_PATH="$1"; shift; }
            ;;
        --force|-f) FORCE="true"; shift ;;
        *) echo -e "\n${RED}Unknown option: $1${NC}"; usage ;;
    esac
done

[[ -z "$MODE" ]] && usage

# **Check Dependencies**
for cmd in xdotool import ffmpeg xwininfo; do
    command -v "$cmd" >/dev/null 2>&1 || {
        echo -e "${RED}Error: $cmd is not installed. Install it (e.g., sudo apt install ${cmd%% *}).${NC}"
        exit 1
    }
done

# **Build VPX File List**
if [[ -n "$SPECIFIC_PATH" ]]; then
    if [[ -d "$SPECIFIC_PATH" ]]; then
        VPX_LIST=$(find "$SPECIFIC_PATH" -maxdepth 1 -type f -name "*.vpx" | head -n 1)
        [[ -z "$VPX_LIST" ]] && { echo -e "${RED}Error: No .vpx file in $SPECIFIC_PATH${NC}"; exit 1; }
    elif [[ -f "$SPECIFIC_PATH" ]]; then
        VPX_LIST="$SPECIFIC_PATH"
    else
        echo -e "${RED}Error: Invalid path '$SPECIFIC_PATH'${NC}"
        exit 1
    fi
else
    VPX_LIST=$(find "$ROOT_FOLDER" -name "*.vpx")
fi

# **Process VPX Files**
echo -e "${GREEN}Processing VPX files...${NC}"
while IFS= read -r VPX_PATH <&3; do
    TABLE_NAME=$(basename "$VPX_PATH" .vpx)
    TABLE_DIR=$(dirname "$VPX_PATH")
    TABLE_VIDEO_FILE="${TABLE_DIR}/${TABLE_VIDEO}"
    TABLE_IMAGE_FILE="${TABLE_DIR}/${TABLE_IMAGE}"
    BACKGLASS_VIDEO_FILE="${TABLE_DIR}/${BACKGLASS_VIDEO}"
    BACKGLASS_IMAGE_FILE="${TABLE_DIR}/${BACKGLASS_IMAGE}"
    DMD_VIDEO_FILE="${TABLE_DIR}/${DMD_VIDEO}"
    DMD_IMAGE_FILE="${TABLE_DIR}/${DMD_IMAGE}"

    VBS_FILE="${TABLE_DIR}/${TABLE_NAME}.vbs"
    CAPTURE_DMD="check_needed"

    echo -e "${BLUE}Processing: $(basename "$TABLE_DIR")${NC}"

    # Skip if media exists and not forcing
    if [[ "$NO_VIDEO" == "false" ]]; then
        [[ "$MODE" == "now" && -f "$TABLE_VIDEO_FILE" && -f "$BACKGLASS_VIDEO_FILE" && -f "$DMD_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All MP4s exist, skipping.${NC}"; continue
        }
        [[ "$MODE" == "tables-only" && -f "$TABLE_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Table MP4 exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Backglass MP4 exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "dmd-only" && -f "$DMD_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}DMD MP4 exists, skipping.${NC}"; continue
        }
    else
        [[ "$MODE" == "now" && -f "$TABLE_IMAGE_FILE" && -f "$BACKGLASS_IMAGE_FILE" && -f "$DMD_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All PNGs exist, skipping.${NC}"; continue
        }
        [[ "$MODE" == "tables-only" && -f "$TABLE_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Table PNG exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Backglass PNG exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "dmd-only" && -f "$DMD_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}DMD PNG exists, skipping.${NC}"; continue
        }
    fi

    if [[ -f "$VBS_FILE" ]]; then
        if has_dmd_from_vbs "$VBS_FILE"; then
            echo "DMD detected in $TABLE_NAME via .vbs"
            CAPTURE_DMD="true"
        else
            echo "No DMD detected in $TABLE_NAME via .vbs"
            CAPTURE_DMD="false"
        fi
    else
        echo "No .vbs file found for $TABLE_NAME, will check via VPX"
        CAPTURE_DMD="check_later"
    fi

    if [[ "$MODE" != "dmd-only" || "$CAPTURE_DMD" != "false" ]]; then
        # Launch VPX
        echo -e "${YELLOW}Launching VPX for $(basename "$TABLE_DIR")${NC}"
        setsid "$VPX_EXECUTABLE" -play "$VPX_PATH" >"$LOG_FILE" 2>&1 &
        VPINBALLX_PID=$!
    else
        echo -e "${YELLOW}Skipping $TABLE_NAME in dmd-only mode (no DMD in .vbs)${NC}"
        continue  # Skip to next table
    fi

    sleep 3
    if ! kill -0 "$VPINBALLX_PID" 2>/dev/null; then
        echo "$(date +"%Y-%m-%d %H:%M:%S") - VPX failed to start" >> "$LOG_FILE"
        echo -e "${RED}Error: VPX failed to start. Check $LOG_FILE${NC}"
        continue
    fi

    echo -e "${GREEN}Waiting $LOAD_DELAY seconds for table to load...${NC}"
    sleep "$LOAD_DELAY"
    echo -e "${YELLOW}Starting screen capture...${NC}"

    # Capture windows
    capture_pids=()

    # Capture Table window
    if [[ "$MODE" == "now" || "$MODE" == "tables-only" ]]; then
        WINDOW_ID_VPX=$(xdotool search --name "$WINDOW_TITLE_VPX" | head -n 1)
        if [[ -n "$WINDOW_ID_VPX" ]]; then
            if [[ "$NO_VIDEO" == "false" ]]; then
                check_file="$TABLE_VIDEO_FILE"
            else
                check_file="$TABLE_IMAGE_FILE"
            fi
            if [[ ! -f "$check_file" || "$FORCE" == "true" ]]; then
                capture_vpx_window "$WINDOW_ID_VPX" "$TABLE_VIDEO_FILE" "$TABLE_IMAGE_FILE" &
                capture_pids+=($!)
                echo -e "Capturing table to ${GREEN}$check_file${NC}"
            else
                echo -e "${YELLOW}Table media already exists, skipping.${NC}"
            fi
        else
            echo -e "${RED}Error: '$WINDOW_TITLE_VPX' not found${NC}"
        fi
    fi

    # Capture Backglass window
    if [[ "$MODE" == "now" || "$MODE" == "backglass-only" ]]; then
        WINDOW_ID_BACKGLASS=$(xdotool search --name "$WINDOW_TITLE_BACKGLASS" | head -n 1)
        if [[ -n "$WINDOW_ID_BACKGLASS" ]]; then
            if [[ "$NO_VIDEO" == "false" ]]; then
                check_file="$BACKGLASS_VIDEO_FILE"
            else
                check_file="$BACKGLASS_IMAGE_FILE"
            fi
            if [[ ! -f "$check_file" || "$FORCE" == "true" ]]; then
                capture_vpx_window "$WINDOW_ID_BACKGLASS" "$BACKGLASS_VIDEO_FILE" "$BACKGLASS_IMAGE_FILE" &
                capture_pids+=($!)
                echo -e "Capturing backglass to ${GREEN}$check_file${NC}"
            else
                echo -e "${YELLOW}Backglass media already exists, skipping.${NC}"
            fi
        else
            echo -e "${RED}Error: '$WINDOW_TITLE_BACKGLASS' not found${NC}"
        fi
    fi

    # Capture DMD window
    if [[ "$MODE" == "now" || "$MODE" == "dmd-only" ]]; then
        if [[ "$CAPTURE_DMD" != "false" ]]; then
            if [[ "$NO_VIDEO" == "false" ]]; then
                check_file="$DMD_VIDEO_FILE"
            else
                check_file="$DMD_IMAGE_FILE"
            fi
            if [[ ! -f "$check_file" || "$FORCE" == "true" ]]; then
                if [[ "$CAPTURE_DMD" == "true" || "$CAPTURE_DMD" == "check_later" ]]; then
                    for dmd_name in "${WINDOW_TITLE_DMD[@]}"; do
                        window_ids=$(xdotool search --name "$dmd_name")
                        if [[ -n "$window_ids" ]]; then
                            for window_id in $window_ids; do
                                if xwininfo -id "$window_id" | grep -q "Map State: IsViewable"; then
                                    capture_vpx_window "$window_id" "$DMD_VIDEO_FILE" "$DMD_IMAGE_FILE" &
                                    capture_pids+=($!)
                                    echo -e "Capturing DMD ($dmd_name) to ${GREEN}$check_file${NC}"
                                    CAPTURE_DMD="done"  # Mark as captured
                                    break 2  # Exit both loops after first visible DMD
                                fi
                            done
                        fi
                    done
                    if [[ "$CAPTURE_DMD" == "check_later" ]]; then
                        echo -e "${YELLOW}No visible DMD windows found for $TABLE_NAME${NC}"
                        CAPTURE_DMD="false"  # No DMD found after checking
                    fi
                fi
            else
                echo -e "${YELLOW}DMD media already exists, skipping.${NC}"
            fi
        else
            echo -e "${YELLOW}Skipping DMD capture (no DMD in .vbs)${NC}"
        fi
    fi

    # Wait for all captures to finish
    for pid in "${capture_pids[@]}"; do
        wait "$pid"
    done

    # Terminate VPX
    echo -e "${YELLOW}Terminating VPX (PGID: $VPINBALLX_PID)${NC}"
    kill -TERM -- -"$VPINBALLX_PID" 2>/dev/null
    sleep 2
    kill -0 -- -"$VPINBALLX_PID" 2>/dev/null && kill -9 -- -"$VPINBALLX_PID" 2>/dev/null

    wait "$VPINBALLX_PID" 2>/dev/null
    [[ $? -ne 0 ]] && {
        echo -e "${RED}Error: VPX exited with error. Check $LOG_FILE${NC}"
        echo "$(date +"%Y-%m-%d %H:%M:%S") - VPX failed" >> "$LOG_FILE"
    }

    echo -e "${GREEN}Finished: $TABLE_NAME${NC}"
done 3< <(echo "$VPX_LIST")

echo -e "${GREEN}All media processed.${NC}"