@echo off
setlocal

rem Resolve script directory (bin is in same directory as script)
set SCRIPT_DIR=%~dp0
rem Remove trailing backslash if exists

echo Compiling shaders...

rem Ensure VULKAN_SDK is set
if "%VULKAN_SDK%"=="" (
    echo Error: VULKAN_SDK environment variable is not set
    exit /b 1
)

rem Vertex shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=vert "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.vert.glsl" -o "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.vert.spv"
if errorlevel 1 (
    echo Error: vertex shader compilation failed
    exit /b 1
)

rem Fragment shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=frag "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.frag.glsl" -o "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.frag.spv"
if errorlevel 1 (
    echo Error: fragment shader compilation failed
    exit /b 1
)

rem Grid vertex shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=vert "%SCRIPT_DIR%\assets\shaders\Builtin.GridShader.vert.glsl" -o "%SCRIPT_DIR%\assets\shaders\Builtin.GridShader.vert.spv"
if errorlevel 1 (
    echo Error: grid vertex shader compilation failed
    exit /b 1
)

rem Grid fragment shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=frag "%SCRIPT_DIR%\assets\shaders\Builtin.GridShader.frag.glsl" -o "%SCRIPT_DIR%\assets\shaders\Builtin.GridShader.frag.spv"
if errorlevel 1 (
    echo Error: grid fragment shader compilation failed
    exit /b 1
)

echo Compiled shaders are in: %SCRIPT_DIR%

endlocal
