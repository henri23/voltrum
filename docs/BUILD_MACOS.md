# Building on macOS

macOS is not an officially supported target yet - it's on the roadmap, and the
code carries the pieces it needs (the renderer enables the Vulkan portability
flag on Apple platforms, and SDL3 builds natively), but it isn't part of the
regular testing. Treat this as experimental. If you try it and something is
broken, that's expected; patches are welcome.

There's no Vulkan driver on macOS in the usual sense. Vulkan runs on top of
Metal through MoltenVK, which ships with the LunarG Vulkan SDK. Once the SDK is
installed the same `build.sh` used on Linux works here.

## What you need

**Xcode command line tools** - this gives you Clang and Git. Install them with:

```bash
xcode-select --install
```

**CMake and Ninja** - the simplest route is [Homebrew](https://brew.sh/). Once
Homebrew is installed:

```bash
brew install cmake ninja
```

If you'd rather not use Homebrew, CMake also has a macOS installer at
[cmake.org/download](https://cmake.org/download/) and Ninja has prebuilt
binaries on its [releases page](https://github.com/ninja-build/ninja/releases).

**Vulkan SDK** - download the macOS installer from
[vulkan.lunarg.com](https://vulkan.lunarg.com/sdk/home#mac). On macOS the SDK is
what provides [MoltenVK](https://github.com/KhronosGroup/MoltenVK) (the
Vulkan-on-Metal layer), the Vulkan loader, and the validation layers - there's
no system Vulkan driver otherwise.

After installing the SDK, activate its environment in your shell so the loader
can find MoltenVK and the layers - the SDK ships a `setup-env.sh` you can source,
which sets `VULKAN_SDK`, `VK_ICD_FILENAMES`, `VK_ADD_LAYER_PATH` and the library
path. CMake's `find_package(Vulkan)` and the runtime both rely on this being
set.

As on the other platforms, the default build is a debug build that requests the
`VK_LAYER_KHRONOS_validation` layer at startup. The SDK includes it, but the
loader only finds it when the SDK environment is active.

## Getting the code

```bash
git clone --recursive https://github.com/henri23/voltrum.git
cd voltrum
```

If you cloned without `--recursive`, or pulled changes that moved a submodule:

```bash
git submodule update --init --recursive
```

Run that after any `git pull` as well - pulling doesn't move submodules on its
own, and an out-of-date ImGui checkout shows up as compile errors in the Vulkan
backend.

## Building

```bash
./build.sh --compiler clang
```

`build.sh` is the same script used on Linux and already handles macOS (for
example its timing helper). Clang is the native toolchain here, so pass
`--compiler clang`. The other flags work too - `--clean`, `--skip-tests`,
`--asan`, `--dynamic`, `--build-system ninja|make`.

The build configures into `bin/`, runs the tests, and builds the client without
launching it.

## Running

```bash
./bin/client/voltrum_client
```

Make sure the Vulkan SDK environment is still active in the shell you run it
from, otherwise the loader won't find MoltenVK or the validation layer.

## When it doesn't work

**CMake can't find Vulkan.** The SDK environment isn't set. Source the SDK's
`setup-env.sh` and check `echo $VULKAN_SDK` prints a path, then configure again
with `--clean`.

**The client aborts at startup over the validation layer.** Same cause - the
SDK environment, which carries the layer path, isn't active in this shell.

**No Vulkan device is found at runtime.** MoltenVK isn't being picked up. Confirm
`VK_ICD_FILENAMES` points at the MoltenVK ICD from the SDK (`setup-env.sh` sets
this) and that `vulkaninfo` reports a device.

**Compile errors in `vulkan_ui_backend.cpp` about unknown ImGui members.** The
ImGui submodule is behind the code; run `git submodule update --init --recursive`.

For Linux and Windows see [BUILD_LINUX.md](BUILD_LINUX.md) and
[BUILD_WINDOWS.md](BUILD_WINDOWS.md).
