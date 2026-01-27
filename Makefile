# Makefile for compiling a C program with GLFW and compiling GLSL shaders

# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Iinclude -g
LDFLAGS = -Lexternal -lglfw3 -lvulkan -lm -g

# GLSL compiler
GLSLC = glslc

# Directories
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
GLSL_SRC_DIR = glsl
SPV_DIR = spv

# Program name
TARGET = program

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Collect all C source files
SRCS = $(call rwildcard,$(SRC_DIR),*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Collect all GLSL shader files
GLSL_SRCS = $(wildcard $(GLSL_SRC_DIR)/*.vert $(GLSL_SRC_DIR)/*.frag $(GLSL_SRC_DIR)/*.tesc $(GLSL_SRC_DIR)/*.tese $(GLSL_SRC_DIR)/*.comp)
SPV_FILES = \
    $(patsubst $(GLSL_SRC_DIR)/%.vert, $(SPV_DIR)/%.vert.spv, \
    $(patsubst $(GLSL_SRC_DIR)/%.frag, $(SPV_DIR)/%.frag.spv, \
    $(patsubst $(GLSL_SRC_DIR)/%.tesc, $(SPV_DIR)/%.tesc.spv, \
    $(patsubst $(GLSL_SRC_DIR)/%.tese, $(SPV_DIR)/%.tese.spv, \
    $(patsubst $(GLSL_SRC_DIR)/%.comp, $(SPV_DIR)/%.comp.spv, $(GLSL_SRCS))))))

# Default target
all: $(TARGET) shaders

# Build C program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C source to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile shaders with glslc
shaders: $(SPV_FILES)

$(SPV_DIR)/%.vert.spv: $(GLSL_SRC_DIR)/%.vert
	@mkdir -p $(SPV_DIR)
	$(GLSLC) $< -o $@

$(SPV_DIR)/%.frag.spv: $(GLSL_SRC_DIR)/%.frag
	@mkdir -p $(SPV_DIR)
	$(GLSLC) $< -o $@

$(SPV_DIR)/%.tesc.spv: $(GLSL_SRC_DIR)/%.tesc
	@mkdir -p $(SPV_DIR)
	$(GLSLC) $< -o $@

$(SPV_DIR)/%.tese.spv: $(GLSL_SRC_DIR)/%.tese
	@mkdir -p $(SPV_DIR)
	$(GLSLC) $< -o $@

$(SPV_DIR)/%.comp.spv: $(GLSL_SRC_DIR)/%.comp
	@mkdir -p $(SPV_DIR)
	$(GLSLC) $< -o $@

# Run program (ensure shaders and binary are built)
run: shaders $(TARGET)
	./$(TARGET)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(SPV_DIR)

# Phony targets
.PHONY: all clean run shaders
