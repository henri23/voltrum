# Voltrum

A modern CAD application for microchip design built with **Vulkan**, **ImGui**, and **SDL3**. Designed with a clean core-client architecture for performance, maintainability, and extensibility.

## Features

- **Modern Vulkan Renderer** - High-performance graphics with validation layers and debugging support
- **Component-Based UI System** - Extensible ImGui interface with dockspace, custom titlebar, and font management
- **Memory Management** - Tagged allocation system with arena allocators for optimal performance
- **Cross-Platform** - Native support for Windows (MSVC) and Linux (GCC/Clang)
- **Asset Pipeline** - Efficient loading for images, fonts, and embedded resources
- **Developer Tools** - Modern build scripts, IDE integration, and comprehensive logging

## Quick Start

### Prerequisites

- A C++20 compiler (GCC 11+, Clang 14+, or MSVC 2019+)
- CMake 3.16 or later
- Ninja (or Make on Linux)
- The Vulkan loader, headers and validation layers - from your distro on Linux,
  or the LunarG Vulkan SDK on Windows and macOS
- Git

The default build is a debug build, and it requires the Vulkan validation
layers to be present at runtime.

### Build

Clone with submodules, then run the build script for your platform.

**Linux / macOS:**
```bash
git clone --recursive https://github.com/henri23/voltrum.git
cd voltrum
./build.sh
```

**Windows:**
```cmd
git clone --recursive https://github.com/henri23/voltrum.git
cd voltrum
build.bat
```

If you already cloned without `--recursive`, or a `git pull` left the
submodules behind, sync them before building:

```bash
git submodule update --init --recursive
```

Run that after pulling changes too. A plain `git pull` updates the source but
does not move the submodules, so the ImGui checkout can fall behind what the
renderer expects and the build fails with errors about missing members in the
Vulkan backend. The command above brings the submodules back in line.

The build writes everything under `bin/`; run the client with
`./bin/client/voltrum_client` (`.exe` on Windows). The scripts don't launch it
for you.

For the full details, including dependencies and troubleshooting, see:
- [Linux Build Guide](docs/BUILD_LINUX.md)
- [Windows Build Guide](docs/BUILD_WINDOWS.md)
- [macOS Build Guide](docs/BUILD_MACOS.md) (experimental)

## Architecture

### Core Engine (`voltrum_core.dll/so`)

The core engine provides fundamental systems and abstractions:

- **Application Framework** - Main loop, initialization, and event handling
- **Platform Layer** - SDL3-based windowing, input, and system integration
- **Vulkan Renderer** - Modern graphics API with command buffer management
- **UI System** - Component-based ImGui framework with dockspace and custom titlebar
- **Asset Management** - Efficient resource loading with caching
- **Memory System** - Tagged allocators and arena memory management

### Client Layer (`voltrum_client`)

The client layer implements application-specific functionality:

- **UI Components** - Custom windows, menus, and interface elements
- **Application Logic** - Game or application-specific code
- **Configuration** - Client-specific settings and initialization

### Key Design Principles

- **Separation of Concerns** - Clear boundaries between engine and application code
- **Component Architecture** - Modular, extensible systems
- **Modern C++** - RAII, type safety, and performance-focused design
- **Cross-Platform** - Consistent behavior across Windows and Linux

## Dependencies

### Core Dependencies
- **[SDL3](https://github.com/libsdl-org/SDL)** - Platform abstraction and window management
- **[ImGui](https://github.com/ocornut/imgui)** - Immediate mode GUI (docking branch)
- **[spdlog](https://github.com/gabime/spdlog)** - Fast, header-only logging library
- **[stb_image](https://github.com/nothings/stb)** - Image loading and manipulation
- **Vulkan SDK** - Modern graphics API

### Build Tools
- **CMake** - Cross-platform build system
- **Ninja** - Fast, small build system (recommended)

All dependencies are managed through git submodules or system packages.

## Development

### Build Scripts

Each platform has one build script that handles configuration and compilation:

- `build.sh` (Linux and macOS) - configures with CMake, builds with Ninja or
  Make, and runs the tests. Takes flags like `--clean`, `--dynamic`,
  `--skip-tests`, `--asan`, `--compiler` and `--build-system`.
- `build.bat` (Windows) - sets up the MSVC environment, then configures and
  builds with Ninja. Takes `--clean`, `--dynamic`, `--skip-tests` and `--asan`.

### IDE Integration

- **compile_commands.json** - Generated automatically for LSP/clangd support
- **CLion/Visual Studio** - Native CMake project support
- **VS Code** - C++ extension with IntelliSense support

### Code Quality

- **Static Assertions** - Compile-time type and size validation
- **Vulkan Validation** - Debug layer integration for graphics debugging
- **Memory Tracking** - Tagged allocation system for leak detection
- **Consistent Style** - `.clang-format` configuration included

## Documentation

- [Contributing Guide](CONTRIBUTING.md) - How to contribute to the project
- [Architecture Overview](docs/ARCHITECTURE.md) - Technical architecture and design
- [Linux Build Guide](docs/BUILD_LINUX.md) - Building on Linux systems
- [Windows Build Guide](docs/BUILD_WINDOWS.md) - Building on Windows systems
- [macOS Build Guide](docs/BUILD_MACOS.md) - Building on macOS (experimental)

## Roadmap

See [TODO.md](docs/TODO.md) for the detailed project roadmap, including:

- Engine core improvements and optimizations
- Advanced rendering features (PBR, shadows, post-processing)
- Additional UI components and themes
- Asset pipeline enhancements
- Platform expansion (macOS, mobile)
- Documentation and tutorials

## License

This project is licensed under the [GPL-3.0 License](LICENSE) - see the LICENSE file for details.

**Note**: GPL-3.0 is a copyleft license that requires derivative works to also be licensed under GPL-3.0. This ensures that improvements to Voltrum remain open source and benefit the entire community.

## Contributing

We welcome contributions from the community! Please see our [Contributing Guide](CONTRIBUTING.md) for detailed information on:

- Setting up the development environment
- Code style and conventions
- Submitting bug reports and feature requests
- Pull request process
- Testing guidelines

For quick contributions, the basic workflow is:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes and test thoroughly
4. Commit your changes (`git commit -m 'Add amazing feature'`)
5. Push to your branch (`git push origin feature/amazing-feature`)
6. Open a Pull Request

---

**Built with ❤️ and modern C-style C++**
