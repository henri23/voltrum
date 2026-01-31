#!/bin/bash

# Usage function
show_usage() {
    echo "Usage: $0 [--dynamic] [--skip-tests] [--asan] [--clean] [--compiler <gcc|clang>] [--build-system <ninja|make>]"
    echo "  --dynamic      Build with dynamic linking (DLL)"
    echo "  --skip-tests   Skip building and running tests"
    echo "  --asan         Enable AddressSanitizer for debugging"
    echo "  --clean        Remove the bin directory before configuring"
    echo "  --compiler     Choose compiler toolchain (gcc or clang)"
    echo "  --build-system Select the build tool (ninja or make)"
    echo "  (default)      Build with static linking and run tests"
    exit 1
}

# Parse command line arguments
LINKING_MODE="STATIC"
RUN_TESTS=true
USE_ASAN=false
CLEAN_BUILD=false
C_COMPILER="gcc"
CXX_COMPILER="g++"
COMPILER_NAME="gcc"
BUILD_SYSTEM="ninja"

while [ $# -gt 0 ]; do
    case "$1" in
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
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --compiler)
            shift
            if [ -z "$1" ]; then
                echo "--compiler requires an argument (gcc or clang)"
                exit 1
            fi
            case "$1" in
                gcc)
                    C_COMPILER="gcc"
                    CXX_COMPILER="g++"
                    COMPILER_NAME="gcc"
                    ;;
                clang)
                    C_COMPILER="clang"
                    CXX_COMPILER="clang++"
                    COMPILER_NAME="clang"
                    ;;
                *)
                    echo "Unsupported compiler '$1'. Use gcc or clang."
                    exit 1
                    ;;
            esac
            shift
            ;;
        --compiler=*)
            value="${1#*=}"
            case "$value" in
                gcc)
                    C_COMPILER="gcc"
                    CXX_COMPILER="g++"
                    COMPILER_NAME="gcc"
                    ;;
                clang)
                    C_COMPILER="clang"
                    CXX_COMPILER="clang++"
                    COMPILER_NAME="clang"
                    ;;
                *)
                    echo "Unsupported compiler '$value'. Use gcc or clang."
                    exit 1
                    ;;
            esac
            shift
            ;;
        --build-system)
            shift
            if [ -z "$1" ]; then
                echo "--build-system requires an argument (ninja or make)"
                exit 1
            fi
            BUILD_SYSTEM=$(printf "%s" "$1" | tr '[:upper:]' '[:lower:]')
            shift
            ;;
        --build-system=*)
            value="${1#*=}"
            BUILD_SYSTEM=$(printf "%s" "$value" | tr '[:upper:]' '[:lower:]')
            shift
            ;;
        --help|-h)
            show_usage
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            ;;
    esac
done

case "$BUILD_SYSTEM" in
    ninja)
        CMAKE_GENERATOR="Ninja"
        BUILD_CMD="ninja"
        CLIENT_BUILD_ARGS="-j12"
        BUILD_TOOL_DESC="Ninja"
        ;;
    make)
        CMAKE_GENERATOR="Unix Makefiles"
        BUILD_CMD="make"
        if command -v nproc >/dev/null 2>&1; then
            JOBS=$(nproc)
        else
            JOBS=4
        fi
        CLIENT_BUILD_ARGS="-j$JOBS"
        BUILD_TOOL_DESC="Unix Makefiles (make)"
        ;;
    *)
        echo "Unsupported build system '$BUILD_SYSTEM'. Use ninja or make."
        exit 1
        ;;
esac

# Portable millisecond timing (works on macOS and Linux)
get_time_ms() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS: use perl for millisecond precision
        perl -MTime::HiRes=time -e 'printf "%.0f\n", time * 1000'
    else
        # Linux: use date with %3N
        date +%s%3N
    fi
}

start_time=$(get_time_ms)

# Function to print status messages without ANSI colors
print_status() {
    local status=$1
    local message=$2
    local prefix="[INFO]"
    case $status in
        "success") prefix="[ OK ]" ;;
        "error") prefix="[ERR ]" ;;
        "warning") prefix="[WARN]" ;;
        "step") prefix="[STEP]" ;;
    esac
    echo "$prefix $message"
}

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ] && [ -d "bin" ]; then
    print_status "step" "Removing existing build directory (clean build)..."
    rm -rf bin
fi

# Ensure the bin directory exists
print_status "step" "Creating build directory..."
mkdir -p bin

# Set CMake linking option based on mode
if [ "$LINKING_MODE" = "STATIC" ]; then
    CMAKE_LINKING_FLAG="-DVOLTRUM_STATIC_LINKING=ON"
    LINKING_DESC="Static (single self-contained executable)"
    LIBRARIES_DESC="Core, SDL3, spdlog, ImGui -> all static"
else
    CMAKE_LINKING_FLAG="-DVOLTRUM_STATIC_LINKING=OFF"
    LINKING_DESC="Dynamic (executable + shared libraries)"
    LIBRARIES_DESC="Core, SDL3 -> shared | spdlog, ImGui -> static"
fi

# Set AddressSanitizer flags
if [ "$USE_ASAN" = true ]; then
    CMAKE_SANITIZER_FLAGS="-DCMAKE_CXX_FLAGS=-fsanitize=address -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
    ASAN_DESC="Enabled (debugging mode)"
else
    CMAKE_SANITIZER_FLAGS=""
    ASAN_DESC="Disabled"
fi

print_status "step" "Configuring project with CMake..."
echo "  Generator: $CMAKE_GENERATOR"
echo "  Build system: $BUILD_TOOL_DESC"
echo "  Compiler: $COMPILER_NAME"
echo "  Build Type: Debug"
echo "  Linking Mode: $LINKING_DESC"
echo "  Libraries: $LIBRARIES_DESC"
echo "  AddressSanitizer: $ASAN_DESC"
if [ "$USE_ASAN" = true ]; then
    echo "  Warning: Performance will be significantly reduced"
fi
echo

if time cmake -G "$CMAKE_GENERATOR" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
    -DCMAKE_C_COMPILER=$C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
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

# Test building and running (BEFORE building the main client)
if [ "$RUN_TESTS" = true ]; then
    print_status "step" "Building voltrum tests..."
    echo "  Target: voltrum_tests"
    echo

    if time $BUILD_CMD voltrum_tests; then
        print_status "success" "Test build completed successfully"
    else
        print_status "error" "Test build failed with exit code $?"
        exit 1
    fi
    echo

    print_status "step" "Running voltrum tests..."

    test_start_time=$(get_time_ms)

    if ./tests/voltrum_tests; then
        test_end_time=$(get_time_ms)
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
echo "  Target: voltrum_client"
echo

if time $BUILD_CMD $CLIENT_BUILD_ARGS voltrum_client; then
    print_status "success" "Build completed successfully"

    echo
    print_status "step" "Build verification..."

    # Show file sizes
    EXECUTABLE_SIZE=$(du -h client/voltrum_client | cut -f1)
    echo "  Executable size: $EXECUTABLE_SIZE"

    if [ "$LINKING_MODE" = "STATIC" ]; then
        # Check that no shared libraries are present
        if [ -f "core/libvoltrum_core.so" ]; then
            print_status "warning" "Unexpected shared library found"
        else
            print_status "success" "No shared libraries generated (as expected)"
        fi

        # Show dependency count
        DEPS_COUNT=$(ldd client/voltrum_client 2>/dev/null | wc -l)
        echo "  Dynamic dependencies: $DEPS_COUNT (system libraries only)"

    else
        # Show shared library sizes
        if [ -f "core/libvoltrum_core.so" ]; then
            CORE_SIZE=$(du -h core/libvoltrum_core.so | cut -f1)
            echo "  Core library size: $CORE_SIZE"
        fi

        if [ -f "external/SDL3/libSDL3.so.0.3.0" ]; then
            SDL_SIZE=$(du -h external/SDL3/libSDL3.so.0.3.0 | cut -f1)
            echo "  SDL3 library size: $SDL_SIZE"
        fi

        DEPS_COUNT=$(ldd client/voltrum_client 2>/dev/null | wc -l)
        echo "  Dynamic dependencies: $DEPS_COUNT (includes project libraries)"
    fi

else
    print_status "error" "Build failed with exit code $?"
    exit 1
fi
echo

# Record end time and calculate duration
end_time=$(get_time_ms)
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
