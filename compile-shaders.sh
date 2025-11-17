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
mkdir -p "$BIN_ASSETS_DIR"
# Copy everything except compiled shader blobs, preserving folder layout.
(
    cd "$SCRIPT_DIR" || exit 1
    find assets -type f ! -name "*.spv" | while read -r file; do
        target_dir="bin/$(dirname "$file")"
        mkdir -p "$target_dir"
        cp "$file" "$target_dir/"
    done
)

echo "Shader compilation and asset copying completed successfully."
echo "Compiled shaders are in: $BIN_SHADERS_DIR"
