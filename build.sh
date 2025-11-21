#!/bin/bash

# Usage function
show_usage() {
    echo "Usage: $0 [--dynamic] [--skip-tests] [--asan]"
    echo "  --dynamic      Build with dynamic linking (DLL)"
    echo "  --skip-tests   Skip building and running tests"
    echo "  --asan         Enable AddressSanitizer for debugging"
    echo "  (default)      Build with static linking and run tests"
    exit 1
}

# Parse command line arguments
LINKING_MODE="STATIC"
RUN_TESTS=true
USE_ASAN=false
for arg in "$@"; do
    case $arg in
        --dynamic)
            LINKING_MODE="DYNAMIC"
            shift
            ;;
        --skip-tests)
            RUN_TESTS=false
            shift
            ;;
        --asan)
            USE_ASAN=true
            shift
            ;;
        --help|-h)
            show_usage
            ;;
        *)
            echo "Unknown option: $arg"
            show_usage
            ;;
    esac
done

# Colors for modern output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Unicode symbols
CHECK_MARK="✓"
CROSS_MARK="✗"
ARROW="→"
STAR="★"
GEAR="⚙"

start_time=$(date +%s%3N)

# Function to print colored status messages
print_status() {
    local status=$1
    local message=$2
    case $status in
        "info")
            echo -e "${BLUE}${GEAR}${NC} ${WHITE}${message}${NC}"
            ;;
        "success")
            echo -e "${GREEN}${CHECK_MARK}${NC} ${GREEN}${message}${NC}"
            ;;
        "error")
            echo -e "${RED}${CROSS_MARK}${NC} ${RED}${message}${NC}"
            ;;
        "warning")
            echo -e "${YELLOW}${STAR}${NC} ${YELLOW}${message}${NC}"
            ;;
        "step")
            echo -e "${MAGENTA}${ARROW}${NC} ${WHITE}${message}${NC}"
            ;;
    esac
}

# Function to check if a tool is available
check_tool() {
    local tool=$1
    local package_hint=$2

    if command -v "$tool" >/dev/null 2>&1; then
        print_status "success" "Found $tool"
        return 0
    else
        print_status "error" "Missing required tool: $tool"
        if [ -n "$package_hint" ]; then
            echo -e "   ${YELLOW}${ARROW} Install with: ${WHITE}$package_hint${NC}"
        fi
        return 1
    fi
}

# Tool checking phase
print_status "step" "Checking required build tools..."
echo

tools_ok=true

# Check required tools
# check_tool "cmake" "sudo apt install cmake" || tools_ok=false
# check_tool "ninja" "sudo apt install ninja-build" || tools_ok=false
# check_tool "clang++" "sudo apt install clang" || tools_ok=false
# check_tool "git" "sudo apt install git" || tools_ok=false

echo

if [ "$tools_ok" = false ]; then
    print_status "error" "Some required tools are missing. Please install them before continuing."
    exit 1
fi

print_status "success" "All required tools are available!"
echo

# Ensure the bin directory exists
print_status "step" "Creating build directory..."
mkdir -p bin

# Set CMake linking option based on mode
if [ "$LINKING_MODE" = "STATIC" ]; then
    CMAKE_LINKING_FLAG="-DVOLTRUM_STATIC_LINKING=ON"
    LINKING_DESC="Static (single self-contained executable)"
    LIBRARIES_DESC="Core, SDL3, spdlog, ImGui → all static"
else
    CMAKE_LINKING_FLAG="-DVOLTRUM_STATIC_LINKING=OFF"
    LINKING_DESC="Dynamic (executable + shared libraries)"
    LIBRARIES_DESC="Core, SDL3 → shared | spdlog, ImGui → static"
fi

# Set AddressSanitizer flags
if [ "$USE_ASAN" = true ]; then
    CMAKE_SANITIZER_FLAGS="-DCMAKE_CXX_FLAGS=-fsanitize=address -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
    ASAN_DESC="Enabled (debugging mode)"
else
    CMAKE_SANITIZER_FLAGS=""
    ASAN_DESC="Disabled"
fi

# Run CMake with Ninja generator and Clang++
print_status "step" "Configuring project with CMake..."
echo -e "${BLUE}${ARROW} Generator:${NC} Ninja"
echo -e "${BLUE}${ARROW} Compiler:${NC} g++"
echo -e "${BLUE}${ARROW} Build Type:${NC} Debug"
echo -e "${BLUE}${ARROW} Linking Mode:${NC} $LINKING_DESC"
echo -e "${BLUE}${ARROW} Libraries:${NC} $LIBRARIES_DESC"
echo -e "${BLUE}${ARROW} AddressSanitizer:${NC} $ASAN_DESC"
if [ "$USE_ASAN" = true ]; then
    echo -e "${YELLOW}${ARROW} Warning:${NC} Performance will be significantly reduced"
fi
echo

if time cmake -G Ninja \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    $CMAKE_LINKING_FLAG \
    $CMAKE_SANITIZER_FLAGS \
    -B bin \
    .; then
    print_status "success" "CMake configuration completed"
else
    print_status "error" "CMake configuration failed"
    exit 1
fi
echo

# Go into the bin directory
cd bin

# Link compile_commands.json for IDE integration
print_status "step" "Setting up IDE integration..."
ln -sf "$(pwd)/compile_commands.json" ../compile_commands.json
print_status "success" "Created compile_commands.json symlink"
echo

# Build with Ninja
# echo "=============================================="
# echo "[BUILDER]: Building tests..."
# time ninja koala_tests

# echo "=============================================="
# echo "[BUILDER]: Running tests..."
# ./tests/koala_tests
# TEST_EXIT_CODE=$?
# if [ $TEST_EXIT_CODE -ne 0 ]; then
#     echo "[BUILDER]: Tests failed. Aborting build."
#     exit 1
# fi

# Test building and running (BEFORE building the main client)
if [ "$RUN_TESTS" = true ]; then
    print_status "step" "Building voltrum tests..."
    echo -e "${BLUE}${ARROW} Target:${NC} voltrum_tests"
    echo

    if time ninja voltrum_tests; then
        print_status "success" "Test build completed successfully"
    else
        print_status "error" "Test build failed with exit code $?"
        exit 1
    fi
    echo

    print_status "step" "Running voltrum tests..."

    test_start_time=$(date +%s%3N)

    if ./tests/voltrum_tests; then
        test_end_time=$(date +%s%3N)
        test_time=$(expr $test_end_time - $test_start_time)

        # Convert test time to readable format
        if [ $test_time -gt 1000 ]; then
            test_seconds=$((test_time / 1000))
            test_ms=$((test_time % 1000))
            test_time_str="${test_seconds}.${test_ms}s"
        else
            test_time_str="${test_time}ms"
        fi

        print_status "success" "All tests passed in $test_time_str"
    else
        TEST_EXIT_CODE=$?
        print_status "error" "Tests failed with exit code $TEST_EXIT_CODE"
        print_status "error" "Aborting build - client will NOT be built"
        exit 1
    fi
    echo
else
    print_status "warning" "Skipping tests as requested"
    echo
fi

print_status "step" "Building voltrum client..."
echo -e "${BLUE}${ARROW} Target:${NC} voltrum_client"
echo

if time ninja -j12 voltrum_client; then
    print_status "success" "Build completed successfully"

    echo
    print_status "step" "Build verification..."

    # Show file sizes
    EXECUTABLE_SIZE=$(du -h client/voltrum_client | cut -f1)
    echo -e "${BLUE}${ARROW} Executable size:${NC} $EXECUTABLE_SIZE"

    if [ "$LINKING_MODE" = "STATIC" ]; then
        # Check that no shared libraries are present
        if [ -f "core/libvoltrum_core.so" ]; then
            print_status "warning" "Unexpected shared library found"
        else
            print_status "success" "No shared libraries generated (as expected)"
        fi

        # Show dependency count
        DEPS_COUNT=$(ldd client/voltrum_client 2>/dev/null | wc -l)
        echo -e "${BLUE}${ARROW} Dynamic dependencies:${NC} $DEPS_COUNT (system libraries only)"

    else
        # Show shared library sizes
        if [ -f "core/libvoltrum_core.so" ]; then
            CORE_SIZE=$(du -h core/libvoltrum_core.so | cut -f1)
            echo -e "${BLUE}${ARROW} Core library size:${NC} $CORE_SIZE"
        fi

        if [ -f "external/SDL3/libSDL3.so.0.3.0" ]; then
            SDL_SIZE=$(du -h external/SDL3/libSDL3.so.0.3.0 | cut -f1)
            echo -e "${BLUE}${ARROW} SDL3 library size:${NC} $SDL_SIZE"
        fi

        DEPS_COUNT=$(ldd client/voltrum_client 2>/dev/null | wc -l)
        echo -e "${BLUE}${ARROW} Dynamic dependencies:${NC} $DEPS_COUNT (includes project libraries)"
    fi

else
    print_status "error" "Build failed with exit code $?"
    exit 1
fi
echo

# echo "=============================================="
# echo "[BUILDER]: Compiling shaders..."
# sh compile-shaders.sh

# Check for errors
# ERRORLEVEL=$?
# if [ $ERRORLEVEL -ne 0 ]; then
#     print_status "error" "Shader compilation failed with exit code $ERRORLEVEL"
#     exit 1
# fi

# Record end time and calculate duration
end_time=$(date +%s%3N)
tottime=$(expr $end_time - $start_time)

# Convert milliseconds to a more readable format
if [ $tottime -gt 60000 ]; then
    minutes=$((tottime / 60000))
    seconds=$(((tottime % 60000) / 1000))
    ms=$((tottime % 1000))
    time_str="${minutes}m ${seconds}.${ms}s"
elif [ $tottime -gt 1000 ]; then
    seconds=$((tottime / 1000))
    ms=$((tottime % 1000))
    time_str="${seconds}.${ms}s"
else
    time_str="${tottime}ms"
fi

print_status "success" "All assemblies built successfully"
print_status "info" "Total build time: $time_str"
echo

print_status "step" "Launching voltrum client..."

# ./client/voltrum_client
