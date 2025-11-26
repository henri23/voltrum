#!/bin/bash

# Resolve script directory (bin is in same directory as script)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHADERS_DIR="$SCRIPT_DIR/assets/shaders"

echo "Compiling shaders..."

# Vertex shader
$VULKAN_SDK/bin/glslc -fshader-stage=vert "$SHADERS_DIR/Builtin.MaterialShader.vert.glsl" -o "$SHADERS_DIR/Builtin.MaterialShader.vert.spv"
if [ $? -ne 0 ]; then
    echo "Error: vertex shader compilation failed"
    exit 1
fi

# Fragment shader
$VULKAN_SDK/bin/glslc -fshader-stage=frag "$SHADERS_DIR/Builtin.MaterialShader.frag.glsl" -o "$SHADERS_DIR/Builtin.MaterialShader.frag.spv"
if [ $? -ne 0 ]; then
    echo "Error: fragment shader compilation failed"
    exit 1
fi

echo "Compiled shaders are in: $SHADERS_DIR/assets"
