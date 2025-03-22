# Makefile for ASAPCabinetFE
# Tarso Galvão, Mar 2025

# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -I$(SDL2_INCLUDE) -I$(SRC_DIR)/imgui -I$(SRC_DIR)/imgui/backends -D_REENTRANT
LDFLAGS = -L$(SDL2_LIB) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2

# Parallel jobs (number of CPU cores)
JOBS ?= $(shell nproc 2>/dev/null || echo 4)  # Fallback to 4 if nproc fails

# Detect OS for portability
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    SDL2_INCLUDE ?= /usr/include/SDL2
    SDL2_LIB ?= /usr/lib
else ifeq ($(UNAME_S),Darwin)  # macOS
    SDL2_INCLUDE ?= /usr/local/include/SDL2
    SDL2_LIB ?= /usr/local/lib
else ifeq ($(OS),Windows_NT)   # Windows (MinGW/MSYS2)
    SDL2_INCLUDE ?= /mingw64/include/SDL2
    SDL2_LIB ?= /mingw64/lib
endif

# Directories
SRC_DIR = src
IMGUI_DIR = $(SRC_DIR)/imgui
BACKENDS_DIR = $(IMGUI_DIR)/backends
OUT_DIR = .
OBJ_DIR = obj

# Source files (only the ones you need)
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) \
          $(IMGUI_DIR)/imgui.cpp \
          $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp \
          $(BACKENDS_DIR)/imgui_impl_sdl2.cpp \
          $(BACKENDS_DIR)/imgui_impl_sdlrenderer2.cpp

# Object files
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SOURCES)))

# Executable name
TARGET = $(OUT_DIR)/ASAPCabinetFE

# Default target: debug build
all: debug

# Debug build with parallel jobs
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@echo "Built debug with -j$(JOBS)"

# Release build with parallel jobs
release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)
	@echo "Built release with -j$(JOBS)"

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(BACKENDS_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Phony targets
.PHONY: all debug release clean

# Enable parallel builds by default
MAKEFLAGS += -j$(JOBS)