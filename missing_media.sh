#!/bin/bash
# ----------------------------------------------------------
# Lists missing media from ASAPCabinetFE
# ----------------------------------------------------------
# Options:
#   --tables              List tables missing 'table.png' image and exit
#   --backglass           List tables missing 'backglass.png' image and exit
#   --wheel               List tables missing 'wheel.png' image and exit
#   --marquee             List tables missing 'marquee.png' image and exit
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
    ROOT_FOLDER=$(get_ini_value "VPX" "TablesPath")
    echo -e "${YELLOW}ROOT_FOLDER: $ROOT_FOLDER${NC}"

    WHEEL_IMAGE=$(get_ini_value "CustomMedia" "WheelImage")
    echo -e "${YELLOW}WHEEL_IMAGE: $WHEEL_IMAGE${NC}"
    
    DMD_IMAGE=$(get_ini_value "CustomMedia" "DmdImage")
    echo -e "${YELLOW}DMD_IMAGE: $DMD_IMAGE${NC}"
    
    BACKGLASS_IMAGE=$(get_ini_value "CustomMedia" "BackglassImage")
    echo -e "${YELLOW}BACKGLASS_IMAGE: $BACKGLASS_IMAGE${NC}"
    
    TABLE_IMAGE=$(get_ini_value "CustomMedia" "TableImage")
    echo -e "${YELLOW}TABLE_IMAGE: $TABLE_IMAGE${NC}"
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
  echo -e "  ${YELLOW}--table                  ${NC}List tables missing table images and exit"
  echo -e "  ${YELLOW}--backglass              ${NC}List tables missing backglass images and exit"
  echo -e "  ${YELLOW}--wheel                  ${NC}List tables missing wheel images and exit"
  echo -e "  ${YELLOW}--marquee                ${NC}List tables missing dmd images and exit"
  echo -e "\n  ${NC}-h, --help               Show this help message and exit"
  echo -e "\n${YELLOW}Note:${NC} No args shows this help."
  exit 0
}

# Check for help or no arguments
if [ "$#" -eq 0 ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    show_help
fi

MODE="none" # Default: no action unless specified

# Parse arguments
while [ "$#" -gt 0 ]; do
    case "$1" in  
        --wheel|--marquee|--table|--backglass)
            case "$1" in
                --wheel) IMAGE_TYPE="$WHEEL_IMAGE" ;;
                --marquee) IMAGE_TYPE="$DMD_IMAGE" ;;
                --table) IMAGE_TYPE="$TABLE_IMAGE" ;;
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