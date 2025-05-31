#!/bin/bash
# ----------------------------------------------------------
# Lists missing media from ASAPCabinetFE
# ----------------------------------------------------------
# Options:
#   --tables              List tables missing 'table.png' image and exit
#   --backglass           List tables missing 'backglass.png' image and exit
#   --wheel               List tables missing 'wheel.png' image and exit
#   --marquee             List tables missing 'marquee.png' image and exit
#   --playfield           List tables missing 'table.mp4' video and exit
#   --b2s                 List tables missing 'backglass.mp4' video and exit
#   --dmd                 List tables missing 'marquee.mp4' video and exit
#   --music               List tables missing 'music.mp3' music and exit
#   -h, --help            Show this help message and exit
# ----------------------------------------------------------
# Tarso Galvão Feb/2025

# ANSI color codes
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE='\033[0;34m'
NC="\033[0m" # No Color

CONFIG_FILE="config.ini"

# Function to get values from INI filef
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
echo -e "${GREEN}Initializing variables...${NC}"
echo -e "${RED}-------------------------------------------------------------${NC}"
if [[ -f "$CONFIG_FILE" ]]; then
    ROOT_FOLDER=$(get_ini_value "VPX" "VPXTablesPath")
    echo -e "${YELLOW}ROOT_FOLDER: $ROOT_FOLDER${NC}"

    WHEEL_IMAGE=$(get_ini_value "CustomMedia" "WheelImage")
    echo -e "${YELLOW}WHEEL_IMAGE: $WHEEL_IMAGE${NC}"
    
    MARQUEE_IMAGE=$(get_ini_value "CustomMedia" "DmdImage")
    echo -e "${YELLOW}MARQUEE_IMAGE: $MARQUEE_IMAGE${NC}"
    
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    echo -e "${YELLOW}BACKGLASS_IMAGE: $BACKGLASS_IMAGE${NC}"
    
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "PlayfieldImage")
    echo -e "${YELLOW}TABLE_IMAGE: $TABLE_IMAGE${NC}"

    TABLE_VIDEO=$(get_ini_value "CustomMedia" "PlayfieldVideo")
    echo -e "${YELLOW}TABLE_VIDEO: $TABLE_VIDEO${NC}"

    BACKGLASS_VIDEO=$(get_ini_value "CustomMedia" "BackglassVideo")
    echo -e "${YELLOW}BACKGLASS_VIDEO: $BACKGLASS_VIDEO${NC}"

    DMD_VIDEO=$(get_ini_value "CustomMedia" "DmdVideo")
    echo -e "${YELLOW}DMD_VIDEO: $DMD_VIDEO${NC}"

    TABLE_MUSIC=$(get_ini_value "CustomMedia" "TableMusic")
    echo -e "${YELLOW}TABLE_MUSIC: $TABLE_MUSIC${NC}"

    echo -e "${RED}-------------------------------------------------------------${NC}"
else
    echo -e "${RED}ERROR: config.ini not found. Exiting...${NC}"
    echo -e "${RED}-------------------------------------------------------------${NC}"
    exit 1
fi


# Help function
show_help() {
  echo -e "${BLUE}Options:${NC}"
  echo ""
  echo -e "  ${YELLOW}--table                  ${NC}List tables missing playfield ${YELLOW}images${NC} and exit"
  echo -e "  ${YELLOW}--backglass              ${NC}List tables missing backglass ${YELLOW}images${NC} and exit"
  echo -e "  ${YELLOW}--wheel                  ${NC}List tables missing wheel ${YELLOW}images${NC} and exit"
  echo -e "  ${YELLOW}--marquee                ${NC}List tables missing marquee(DMD) ${YELLOW}images${NC} and exit"
  echo -e "  ${GREEN}--playfield              ${NC}List tables missing playfield ${GREEN}videos${NC} and exit"
  echo -e "  ${GREEN}--b2s                    ${NC}List tables missing backglass ${GREEN}videos${NC} and exit"
  echo -e "  ${GREEN}--dmd                    ${NC}List tables missing DMD ${GREEN}videos${NC} and exit"
  echo -e "  ${GREEN}--music                  ${NC}List tables missing ${GREEN}music${NC} and exit"
  echo -e "\n  ${NC}-h, --help               Show this help message and exit"
  echo -e "\n${YELLOW}Note:${NC} No args shows this help."
  exit 0
}

# Check for help or no arguments
if [ "$#" -eq 0 ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    show_help
fi

# Parse arguments
while [ "$#" -gt 0 ]; do
    case "$1" in  
        --wheel|--marquee|--table|--backglass)
            case "$1" in
                --wheel) IMAGE_TYPE="$WHEEL_IMAGE" ;;
                --marquee) IMAGE_TYPE="$MARQUEE_IMAGE" ;;
                --table) IMAGE_TYPE="$TABLE_IMAGE" ;;
                --backglass) IMAGE_TYPE="$BACKGLASS_IMAGE" ;;
            esac

            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$IMAGE_TYPE...${NC}\n"

            missing_count=0

            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    image_file="$table_dir/$IMAGE_TYPE"
                    if [ ! -f "$image_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                        ((missing_count++))
                    fi
                fi
            done

            if [ $missing_count == 0 ]; then
                echo -e "\n${BLUE}Woot! Missing ${GREEN}$missing_count${BLUE} images!${NC}"
            else
                echo -e "\n${BLUE}Missing ${RED}$missing_count${BLUE} image(s).${NC}"
                echo -e "${BLUE}These tables have ${RED}no <table_folder>/$IMAGE_TYPE${BLUE} images.${NC}"
                echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$IMAGE_TYPE${NC}"
            fi
            exit 0
            ;;
        --dmd|--playfield|--b2s)
            case "$1" in
                --dmd) VIDEO_TYPE="$DMD_VIDEO" ;;
                --playfield) VIDEO_TYPE="$TABLE_VIDEO" ;;
                --b2s) VIDEO_TYPE="$BACKGLASS_VIDEO" ;;
            esac

            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$VIDEO_TYPE...${NC}\n"

            missing_count=0

            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    mp4_file="$table_dir/$VIDEO_TYPE"
                    if [ ! -f "$mp4_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                        ((missing_count++))
                    fi
                fi
            done

            if [ $missing_count == 0 ]; then
                echo -e "\n${BLUE}Woot! Missing ${GREEN}$missing_count${BLUE} videos!${NC}"
            else
                echo -e "\n${BLUE}Missing ${RED}$missing_count${BLUE} video(s).${NC}"
                echo -e "${BLUE}These tables have ${RED}no <table_folder>/$VIDEO_TYPE${BLUE} videos.${NC}"
                echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$VIDEO_TYPE${NC}"
            fi
            exit 0
            ;;
        --music)
            echo -e "${BLUE}Using tables directory: ${GREEN}$ROOT_FOLDER"
            echo -e "${BLUE}Checking for tables missing ${GREEN}<table_folder>/$TABLE_MUSIC...${NC}\n"

            missing_count=0

            for vpx_file in "$ROOT_FOLDER"/*/*.vpx; do
                if [ -f "$vpx_file" ]; then
                    table_dir=$(dirname "$vpx_file")
                    music_file="$table_dir/$TABLE_MUSIC"
                    if [ ! -f "$music_file" ]; then
                        echo -e "${GREEN}->${YELLOW} '$(basename "$table_dir")'${NC}"
                        ((missing_count++))
                    fi
                fi
            done

            if [ $missing_count == 0 ]; then
                echo -e "\n${BLUE}Woot! Missing ${GREEN}$missing_count${BLUE} music files!${NC}"
            else
                echo -e "\n${BLUE}Missing ${RED}$missing_count${BLUE} music file(s).${NC}"
                echo -e "${BLUE}These tables have ${RED}no <table_folder>/$TABLE_MUSIC${BLUE} music files.${NC}"
                echo -e "${BLUE}Place them in ${YELLOW}$ROOT_FOLDER<table_folder>/$TABLE_MUSIC${NC}"
            fi
            exit 0
            ;;
        *)
            echo -e "\n${RED}Unknown option: $1${NC}"
            show_help
            ;;
    esac
done
