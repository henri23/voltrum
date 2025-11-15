#!/bin/bash

# Resolve script directory (bin is in same directory as script)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_ASSETS_DIR="$SCRIPT_DIR/bin/assets"
BIN_SHADERS_DIR="$BIN_ASSETS_DIR/shaders"

# Create output directories
mkdir -p "$BIN_SHADERS_DIR"

echo "Compiling shaders..."

# Vertex shader
$VULKAN_SDK/bin/glslc -fshader-stage=vert "$SCRIPT_DIR/assets/shaders/Builtin.MaterialShader.vert.glsl" -o "$BIN_SHADERS_DIR/Builtin.MaterialShader.vert.spv"
if [ $? -ne 0 ]; then
    echo "Error: vertex shader compilation failed"
    exit 1
fi

# Fragment shader
$VULKAN_SDK/bin/glslc -fshader-stage=frag "$SCRIPT_DIR/assets/shaders/Builtin.MaterialShader.frag.glsl" -o "$BIN_SHADERS_DIR/Builtin.MaterialShader.frag.spv"
if [ $? -ne 0 ]; then
    echo "Error: fragment shader compilation failed"
    exit 1
fi

echo "Copying non-shader assets..."
# Copy assets but exclude compiled shader files to prevent overwriting them
# Use find to copy everything except .spv files
mkdir -p "$SCRIPT_DIR/bin/assets"
find "$SCRIPT_DIR/assets" -type f ! -name "*.spv" -exec cp --parents {} "$SCRIPT_DIR/bin/" \;

echo "Shader compilation and asset copying completed successfully."
echo "Compiled shaders are in: $BIN_SHADERS_DIR"
