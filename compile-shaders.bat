@echo off
setlocal

rem Resolve script directory (bin is in same directory as script)
set SCRIPT_DIR=%~dp0
rem Remove trailing backslash if exists
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

set BIN_ASSETS_DIR=%SCRIPT_DIR%\bin\assets
set BIN_SHADERS_DIR=%BIN_ASSETS_DIR%\shaders

rem Create output directories
if not exist "%BIN_SHADERS_DIR%" mkdir "%BIN_SHADERS_DIR%"

echo Compiling shaders...

rem Ensure VULKAN_SDK is set
if "%VULKAN_SDK%"=="" (
    echo Error: VULKAN_SDK environment variable is not set
    exit /b 1
)

rem Vertex shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=vert "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.vert.glsl" -o "%BIN_SHADERS_DIR%\Builtin.MaterialShader.vert.spv"
if errorlevel 1 (
    echo Error: vertex shader compilation failed
    exit /b 1
)

rem Fragment shader
"%VULKAN_SDK%\Bin\glslc.exe" -fshader-stage=frag "%SCRIPT_DIR%\assets\shaders\Builtin.MaterialShader.frag.glsl" -o "%BIN_SHADERS_DIR%\Builtin.MaterialShader.frag.spv"
if errorlevel 1 (
    echo Error: fragment shader compilation failed
    exit /b 1
)

echo Copying assets...
xcopy /E /I /Y "%SCRIPT_DIR%\assets" "%SCRIPT_DIR%\bin\assets"

echo Done.
endlocal
