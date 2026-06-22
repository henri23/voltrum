# Building on Linux

Voltrum builds with a single script, `build.sh`, which configures the project
with CMake and compiles it with Ninja (or Make). The defaults give you a debug,
statically linked build that also runs the test suite before producing the
client.

## What you need

- A C++20 compiler - [GCC](https://gcc.gnu.org/) 11+ or
  [Clang](https://clang.llvm.org/) 14+
- [CMake](https://cmake.org/) 3.16 or newer
- [Ninja](https://ninja-build.org/) (or Make, if you prefer)
- [Git](https://git-scm.com/)
- The Vulkan loader, headers, and **validation layers** (the
  [Vulkan SDK](https://vulkan.lunarg.com/) if you don't use distro packages)
- SDL3's video dependencies (X11 and/or Wayland development headers)

On Linux the easiest way to get all of these is your distribution's package
manager - pick your distro below and the one command installs everything. The
Vulkan SDK link above is only needed if you'd rather install LunarG's SDK by
hand instead of using distro packages.

The validation layers matter: the default build is a debug build, and it asks
the Vulkan loader for `VK_LAYER_KHRONOS_validation` at startup. If that layer
isn't installed the client aborts immediately on `vkCreateInstance`, so don't
skip it.

You don't need any audio libraries. SDL's audio, joystick, haptic and similar
subsystems are turned off in the top-level `CMakeLists.txt`, so the usual ALSA
/ PulseAudio / JACK packages are irrelevant here.

### Arch

```bash
sudo pacman -S base-devel cmake ninja git \
    vulkan-headers vulkan-icd-loader vulkan-validation-layers vulkan-tools \
    libx11 libxext wayland wayland-protocols
```

Add the driver for your GPU: `vulkan-radeon` (AMD), `vulkan-intel` (Intel), or
`nvidia-utils` (NVIDIA).

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
    libvulkan-dev vulkan-validationlayers vulkan-tools \
    libx11-dev libxext-dev libwayland-dev wayland-protocols
```

On AMD/Intel install `mesa-vulkan-drivers`; on NVIDIA use the proprietary
driver package for your card.

### Fedora

```bash
sudo dnf install gcc-c++ cmake ninja-build git \
    vulkan-loader-devel vulkan-validation-layers vulkan-tools \
    libX11-devel libXext-devel wayland-devel wayland-protocols-devel
```

### A note on a hand-installed Vulkan SDK

If you use the LunarG SDK from your home directory instead of distro packages,
the loader only finds the validation layers when the SDK environment is active
(`VK_LAYER_PATH` / `VK_ADD_LAYER_PATH` pointing at the SDK, and its `lib`
directory on `LD_LIBRARY_PATH`). Source the SDK's `setup-env.sh` in your shell,
or make sure those variables are set, otherwise the debug build will fail to
find the layer even though it's installed.

## Getting the code

The project pulls SDL3, ImGui, ImPlot and spdlog in as submodules, so clone
recursively:

```bash
git clone --recursive https://github.com/henri23/voltrum.git
cd voltrum
```

If you already cloned without `--recursive`, or if you pulled changes that moved
a submodule, sync them before building:

```bash
git submodule update --init --recursive
```

This second step matters more than it looks. A plain `git pull` updates the
source but does not move submodules, so the ImGui checkout can fall behind what
the renderer expects and you'll get compile errors about missing members in the
Vulkan backend. When that happens, run the command above.

## Building

```bash
./build.sh
```

That configures into `bin/`, builds and runs the tests, then builds the client.
The script does not launch anything when it's done.

The flags it understands:

- `--dynamic` - build the core and SDL3 as shared libraries instead of one
  static executable
- `--skip-tests` - don't build or run the test suite
- `--asan` - compile with AddressSanitizer (much slower, for debugging)
- `--clean` - delete `bin/` before configuring
- `--compiler gcc|clang` - pick the toolchain (GCC is the default)
- `--build-system ninja|make` - pick the build tool (Ninja is the default)

So a clean Clang build without tests is:

```bash
./build.sh --clean --compiler clang --skip-tests
```

## Running

The build writes everything under `bin/`:

```bash
./bin/client/voltrum_client
```

A static build (the default) is self-contained. If you built with `--dynamic`,
the client needs to find `libvoltrum_core.so` and `libSDL3.so` next to it, so
run it from inside `bin/` or set `LD_LIBRARY_PATH` to include `bin/core` and
`bin/external/SDL3`.

## When it doesn't work

**The client aborts at startup with a validation-layer error.** The layer
isn't installed, or your hand-installed SDK isn't on the environment. See the
notes above - install `vulkan-validation-layers` (or whatever your distro calls
it), or activate the SDK.

**Compile errors in `vulkan_ui_backend.cpp` about unknown ImGui members.** Your
ImGui submodule is out of date relative to the code. Run
`git submodule update --init --recursive`.

**`vkCreateInstance` fails with no usable device.** Your GPU driver doesn't
expose Vulkan. Install the right driver package for your card and check
`vulkaninfo --summary` lists your GPU.

For the Windows and macOS instructions see
[BUILD_WINDOWS.md](BUILD_WINDOWS.md) and [BUILD_MACOS.md](BUILD_MACOS.md).
