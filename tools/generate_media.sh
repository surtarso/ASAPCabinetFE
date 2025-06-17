#!/bin/bash
# ----------------------------------------------------------
# Creates images and videos (playfield + backglass + DMD) for ASAPCabinetFE
# Opens all tables and screenshots playfield, backglass, and visible DMD windows
# Saves them in table_name/images|video/ folder as specified in settings.json
# ----------------------------------------------------------
# Dependencies: xdotool, ImageMagick, ffmpeg, xwininfo
# Author: Tarso Galvao, Feb/2025

# Uncomment the line below for debugging
# set -x

# **Color Codes for Output**
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
NC="\033[0m" # No Color

# **File Paths**
CONFIG_FILE="data/settings.json"

mkdir -p logs/
ERROR_LOG_FILE="logs/error.log"
VPX_LOG_FILE="logs/VPinballX.log"
CAPTURE_LOG="logs/capture.log"

# **Configuration Variables**
LOAD_DELAY=12 # Seconds to wait after launching VPX for tables to load
SCREENSHOT_FPS=12 # Frames per second for video
RECORDING_DURATION=4 # Seconds of video recording
FRAME_COUNT=$((RECORDING_DURATION * SCREENSHOT_FPS))
FRAME_INTERVAL=0.1 # Seconds between screenshots (approx 10 FPS, adjusted by ffmpeg)

WINDOW_TITLE_VPX="Visual Pinball Player"
# These will be set based on VPX_VERSION
WINDOW_TITLE_BACKGLASS=""
WINDOW_TITLE_DMD=()
NODMDFOUND_FILE="noDMDfound.txt"

# Default VPX version, can be overridden by command-line
VPX_VERSION="10.8.0" # Default to 10.8.0 if not specified

# Reads a value from the specified section and key in the INI file
get_json_value() {
    local section="$1"
    local key="$2"

    if [[ ! -f "$CONFIG_FILE" ]]; then
        echo "Error: CONFIG_FILE not found: $CONFIG_FILE" >&2
        return 1
    fi

    jq -r --arg section "$section" --arg key "$key" '.[$section][$key] // empty' "$CONFIG_FILE"
}

# **Load Configuration from settings.json**
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_json_value "VPX" "VPXTablesPath")
    VPX_EXECUTABLE=$(get_json_value "VPX" "VPinballXPath")
    TABLE_VIDEO=$(get_json_value "CustomMedia" "customPlayfieldVideo")
    TABLE_IMAGE=$(get_json_value "CustomMedia" "customPlayfieldImage")
    BACKGLASS_VIDEO=$(get_json_value "CustomMedia" "customBackglassVideo")
    BACKGLASS_IMAGE=$(get_json_value "CustomMedia" "customBackglassImage")
    DMD_VIDEO=$(get_json_value "CustomMedia" "customDmdVideo")
    DMD_IMAGE=$(get_json_value "CustomMedia" "customDmdImage")
    echo -e "${GREEN}Loaded settings.json${NC}"
else
    echo -e "${RED}-------------------------------------------------------------${NC}"
    echo -e "${RED}Error: settings.json not found. Exiting...${NC}"
    echo -e "${RED}-------------------------------------------------------------${NC}"
    exit 1
fi

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
            import -window "$window_id" "$frame_file" >> $CAPTURE_LOG
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
            "$video_output" >> $CAPTURE_LOG 2>&1

        rm -rf "$tmp_dir"
    fi
}

# Function to check if .vbs indicates a DMD
has_dmd_from_vbs() {
    local vbs_file="$1"
    if grep -q -i -E "FlexDMD|B2SDMD|PinMAME|UseDMD|Controller.DMD|UltraDMD|ShowDMDOnly=1" "$vbs_file"; then
        return 0 # DMD present
    else
        return 1 # No DMD
    fi
}

# Displays help message
usage() {
    echo -e "\nCreates ${GREEN}MP4 videos and PNG images ${YELLOW}(playfield + backglass + DMD)${NC} for \033[4mASAPCabinetFE\033[0m"
    echo -e "Saves them in ${YELLOW}tables/<table_folder>/${NC} per ${YELLOW}settings.json${NC}"
    echo -e "\n${BLUE}Usage:${NC} $0 [${BLUE}--missing${NC}|${BLUE}-m${NC}] [${YELLOW}--tables-only${NC}|${YELLOW}-t${NC} [<table_path>] | ${YELLOW}--backglass-only${NC}|${YELLOW}-b${NC} [<table_path>] | ${YELLOW}--dmd-only${NC}|${YELLOW}-d${NC} [<table_path>]] [${GREEN}--image-only${NC}|${GREEN}-i${NC}] [${RED}--force${NC}|${RED}-f${NC}] [${BLUE}--vpx-version ${GREEN}<version>${NC}]"
    echo -e "\nOptions:"
    echo -e "  ${BLUE}--missing, -m          Capture missing table, backglass, and DMD media"
    echo -e "  ${YELLOW}--tables-only, -t      Capture only table media (optional path)"
    echo -e "  ${YELLOW}--backglass-only, -b   Capture only backglass media (optional path)"
    echo -e "  ${YELLOW}--dmd-only, -d         Capture only DMD media (optional path)"
    echo -e "  ${GREEN}--image-only, -i       Capture images only (optional path)"
    echo -e "  ${RED}--force, -f            Force rebuild even if files exist"
    echo -e "  ${BLUE}--vpx-version <version>  Specify VPX version (e.g., 10.8.0, 10.8.1). Default: ${VPX_VERSION}"
    echo -e "  ${NC}-h, --help             Show this help"
    echo -e "\n${YELLOW}Note:${NC} Combine options as needed"
    exit 1
}

# **Parse Command-Line Arguments**
MODE=""
SPECIFIC_PATH=""
FORCE="false"
NO_VIDEO="false"

# Temporarily store original args to re-parse after setting VPX_VERSION
ORIGINAL_ARGS=("$@")

# First pass to get VPX_VERSION
while [[ $# -gt 0 ]]; do
    case "$1" in
        --vpx-version)
            shift
            if [[ -n "$1" && "$1" != -* ]]; then
                VPX_VERSION="$1"
                shift
            else
                echo -e "${RED}Error: --vpx-version requires a version number (e.g., 10.8.0).${NC}"
                usage
            fi
            ;;
        *) shift ;;
    esac
done

# Set specific window titles based on VPX_VERSION
case "$VPX_VERSION" in
    "10.8.0")
        WINDOW_TITLE_BACKGLASS="B2SBackglass"
        WINDOW_TITLE_DMD=("Score" "FlexDMD" "PinMAME" "B2SDMD") # "Score" is at the top, as it seems to be working
        echo -e "${BLUE}Configured for VPX 10.8.0${NC}"
        ;;
    "10.8.1")
        WINDOW_TITLE_BACKGLASS="Backglass"
        WINDOW_TITLE_DMD=("Score") # Only "Score" for 10.8.1
        echo -e "${BLUE}Configured for VPX 10.8.1${NC}"
        ;;
    *)
        echo -e "${RED}Error: Unsupported VPX version: ${VPX_VERSION}. Use 10.8.0 or 10.8.1.${NC}"
        usage
        ;;
esac

# Re-parse original arguments for mode, path, force, image-only
set -- "${ORIGINAL_ARGS[@]}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) usage ;; # Already handled, but keep for consistency
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
        --vpx-version) shift 2 ;; # Consume both option and its value
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
    NODMD_FILE_IMAGE_DIR="${TABLE_DIR}/images/${NODMDFOUND_FILE}"
    NODMD_FILE_VIDEO_DIR="${TABLE_DIR}/video/${NODMDFOUND_FILE}"
    if [[ "$NO_VIDEO" == "false" ]]; then
        [[ "$MODE" == "now" && -f "$TABLE_VIDEO_FILE" && -f "$BACKGLASS_VIDEO_FILE" && -f "$DMD_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All MP4s exist, skipping.${NC}"; continue
        }
        [[ "$MODE" == "now" && -f "$TABLE_VIDEO_FILE" && -f "$BACKGLASS_VIDEO_FILE" && (-f "$NODMD_FILE_IMAGE_DIR" || -f "$NODMD_FILE_VIDEO_DIR") && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All MP4s exist and no DMD (noDMDfound.txt found), skipping.${NC}"; continue
        }
        [[ "$MODE" == "tables-only" && -f "$TABLE_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Table MP4 exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_VIDEO_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Backglass MP4 exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "dmd-only" && (-f "$DMD_VIDEO_FILE" || -f "$NODMD_FILE_IMAGE_DIR" || -f "$NODMD_FILE_VIDEO_DIR") && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}DMD MP4 exists or no DMD (noDMDfound.txt found), skipping.${NC}"; continue
        }
    else
        [[ "$MODE" == "now" && -f "$TABLE_IMAGE_FILE" && -f "$BACKGLASS_IMAGE_FILE" && -f "$DMD_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All PNGs exist, skipping.${NC}"; continue
        }
        [[ "$MODE" == "now" && -f "$TABLE_IMAGE_FILE" && -f "$BACKGLASS_IMAGE_FILE" && (-f "$NODMD_FILE_IMAGE_DIR" || -f "$NODMD_FILE_VIDEO_DIR") && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}All PNGs exist and no DMD (noDMDfound.txt found), skipping.${NC}"; continue
        }
        [[ "$MODE" == "tables-only" && -f "$TABLE_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Table PNG exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "backglass-only" && -f "$BACKGLASS_IMAGE_FILE" && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}Backglass PNG exists, skipping.${NC}"; continue
        }
        [[ "$MODE" == "dmd-only" && (-f "$DMD_IMAGE_FILE" || -f "$NODMD_FILE_IMAGE_DIR" || -f "$NODMD_FILE_VIDEO_DIR") && "$FORCE" != "true" ]] && {
            echo -e "${YELLOW}DMD PNG exists or no DMD (noDMDfound.txt found), skipping.${NC}"; continue
        }
    fi

    # Check VBScript if there's a DMD present
    # For VPX 10.8.1, we assume DMD is always present due to "Score" window
    if [[ "$VPX_VERSION" == "10.8.1" ]]; then
        echo "Assuming DMD is always present for VPX 10.8.1, skipping .vbs check."
        CAPTURE_DMD="true"
    elif [[ -f "$VBS_FILE" ]]; then
        if has_dmd_from_vbs "$VBS_FILE"; then
            echo "DMD detected in $TABLE_NAME via .vbs"
            CAPTURE_DMD="true"
        else
            echo "No DMD detected in $TABLE_NAME via .vbs"
            CAPTURE_DMD="false"
        fi
    else
        echo "No .vbs file found for $TABLE_NAME, will check for a DMD via VPX"
        CAPTURE_DMD="check_later"
    fi

    # In missing mode, skip DMD capture if noDMDfound.txt exists
    if [[ "$MODE" == "now" && (-f "$NODMD_FILE_IMAGE_DIR" || -f "$NODMD_FILE_VIDEO_DIR") ]]; then
        echo -e "${YELLOW}Skipping DMD capture for $TABLE_NAME (noDMDfound.txt found)${NC}"
        CAPTURE_DMD="false"
    fi

    # In missing mode, skip launching VPX if all table/backglass media exists and no DMD capture needed
    if [[ "$MODE" == "now" && "$CAPTURE_DMD" == "false" ]]; then
        if [[ "$NO_VIDEO" == "false" ]]; then
            [[ -f "$TABLE_VIDEO_FILE" && -f "$BACKGLASS_VIDEO_FILE" && "$FORCE" != "true" ]] && {
                echo -e "${YELLOW}All MP4s exist and no DMD (.vbs), skipping.${NC}"; continue
            }
        else
            [[ -f "$TABLE_IMAGE_FILE" && -f "$BACKGLASS_IMAGE_FILE" && "$FORCE" != "true" ]] && {
                echo -e "${YELLOW}All PNGs exist and no DMD (.vbs), skipping.${NC}"; continue
            }
        fi
    fi

    # In DMD-only mode, skip launching VPX if no DMD detected in VBScript
    # This condition must still apply for 10.8.0 if DMD is explicitly 'false'
    if [[ "$MODE" != "dmd-only" || "$CAPTURE_DMD" != "false" ]]; then
        # Launch VPX
        echo -e "${YELLOW}Launching VPX for $(basename "$TABLE_DIR")${NC}"
        setsid "$VPX_EXECUTABLE" -play "$VPX_PATH" >"$VPX_LOG_FILE" 2>&1 &
        VPINBALLX_PID=$!
    else
        echo -e "${YELLOW}Skipping $TABLE_NAME in dmd-only mode (no DMD in .vbs or forced no DMD for 10.8.1)${NC}"
        continue # Skip to next table
    fi

    sleep 5 # check for some start error on vpinballx side
    if ! kill -0 "$VPINBALLX_PID" 2>/dev/null; then
        echo "$(date +"%Y-%m-%d %H:%M:%S") - VPX failed to start" >> "$ERROR_LOG_FILE"
        echo -e "${RED}Error: VPX failed to start. Check $ERROR_LOG_FILE${NC}"
        continue # Skip to next table
    fi

    echo -e "${GREEN}Waiting $LOAD_DELAY seconds for table to load...${NC}"
    sleep "$LOAD_DELAY"
    echo -e "${YELLOW}Starting screen capture...${NC}"

    # Capture windows PIDs
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

    # Capture DMD windows
    if [[ "$MODE" == "now" || "$MODE" == "dmd-only" ]]; then
        if [[ "$CAPTURE_DMD" != "false" ]]; then
            if [[ "$NO_VIDEO" == "false" ]]; then
                base_file="$DMD_VIDEO_FILE" # e.g., video/dmd.mp4
            else
                base_file="$DMD_IMAGE_FILE" # e.g., images/dmd.png
            fi
            if [[ "$CAPTURE_DMD" == "true" || "$CAPTURE_DMD" == "check_later" ]]; then
                dmd_count=0 # Counter for naming additional DMDs
                for dmd_name in "${WINDOW_TITLE_DMD[@]}"; do
                    window_ids=$(xdotool search --name "$dmd_name")
                    if [[ -n "$window_ids" ]]; then
                        for window_id in $window_ids; do
                            if xwininfo -id "$window_id" | grep -q "Map State: IsViewable"; then
                                # Determine output file name based on dmd_count
                                if [[ $dmd_count -eq 0 ]]; then
                                    check_file="$base_file"
                                    video_output="$DMD_VIDEO_FILE"
                                    image_output="$DMD_IMAGE_FILE"
                                else
                                    # Construct extra file names (e.g., dmd_extra, dmd_extra2)
                                    extra_suffix="_extra"
                                    [[ $dmd_count -gt 1 ]] && extra_suffix="_extra$dmd_count"
                                    check_file="${base_file%.*}${extra_suffix}.${base_file##*.}"
                                    video_output="${DMD_VIDEO_FILE%.*}${extra_suffix}.${DMD_VIDEO_FILE##*.}"
                                    image_output="${DMD_IMAGE_FILE%.*}${extra_suffix}.${DMD_IMAGE_FILE##*.}"
                                fi

                                # Capture if file doesn’t exist or force is enabled
                                if [[ ! -f "$check_file" || "$FORCE" == "true" ]]; then
                                    capture_vpx_window "$window_id" "$video_output" "$image_output" &
                                    capture_pids+=($!)
                                    echo -e "Capturing DMD ($dmd_name) to ${GREEN}$check_file${NC}"
                                    CAPTURE_DMD="done" # Mark that at least one DMD was captured
                                else
                                    echo -e "${YELLOW}DMD media ($dmd_name) already exists at $check_file, skipping.${NC}"
                                fi
                                ((dmd_count++)) # Increment counter for next DMD
                            fi
                        done
                    fi
                done
                if [[ $dmd_count -eq 0 ]]; then
                    echo -e "${YELLOW}No visible DMD windows found for $TABLE_NAME${NC}"
                    # Create noDMDfound.txt in both image and video directories
                    NODMD_FILE_IMAGE_DIR="${TABLE_DIR}/images/${NODMDFOUND_FILE}"
                    NODMD_FILE_VIDEO_DIR="${TABLE_DIR}/video/${NODMDFOUND_FILE}"
                    mkdir -p "${TABLE_DIR}/images" "${TABLE_DIR}/video"
                    echo -e "Table: $TABLE_NAME\nPath: $VPX_PATH\nThis table was tagged as a NO-DMD table, dmd capture will be skipped unless this file is deleted." > "$NODMD_FILE_IMAGE_DIR"
                    echo -e "Table: $TABLE_NAME\nPath: $VPX_PATH\nThis table was tagged as a NO-DMD table, dmd capture will be skipped unless this file is deleted." > "$NODMD_FILE_VIDEO_DIR"
                    echo -e "${YELLOW}Created $NODMDFOUND_FILE for $TABLE_NAME in images/ and video/ directories${NC}"
                    CAPTURE_DMD="false" # No DMDs found after checking
                fi
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
        echo -e "${RED}Error: VPX exited with error. Check $ERROR_LOG_FILE${NC}"
        echo "$(date +"%Y-%m-%d %H:%M:%S") - VPX failed to exit gracefully. Status: $?" >> "$ERROR_LOG_FILE"
    }

    echo -e "${GREEN}Finished: $TABLE_NAME${NC}"
done 3< <(echo "$VPX_LIST")

echo -e "${GREEN}All media processed.${NC}"