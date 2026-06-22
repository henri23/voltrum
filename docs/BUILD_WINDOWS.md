# Building on Windows

On Windows the project builds with MSVC, driven by `build.bat`. The script
finds your Visual Studio installation, sets up the compiler environment, then
configures with CMake and compiles with Ninja. As on Linux, the default is a
debug, statically linked build that runs the tests before producing the client.

## What you need

You'll install four things: a C++ compiler (MSVC), CMake, Ninja, and the Vulkan
SDK - plus Git if you don't already have it.

**Visual Studio 2019 or 2022** - this is where MSVC comes from. Download the
Community edition (free) from
[visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/).
In the installer, check the **"Desktop development with C++"** workload before
installing; that pulls in the compiler, the Windows SDK, and `vcvarsall.bat`,
which is what `build.bat` uses to set up the compiler environment. If you only
want the toolchain without the full IDE, the
[Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
installer offers the same workload.

**CMake 3.16 or newer** - get the Windows installer from
[cmake.org/download](https://cmake.org/download/) and let it add CMake to your
`PATH` (there's a checkbox for it during install). Verify with `cmake --version`.

**Ninja** - the Visual Studio C++ workload already includes it, so you may not
need a separate install. If `ninja --version` doesn't work in a normal terminal,
download `ninja.exe` from the
[Ninja releases page](https://github.com/ninja-build/ninja/releases), put it in a
folder, and add that folder to your `PATH`.

**Vulkan SDK** - download and run the installer from
[vulkan.lunarg.com](https://vulkan.lunarg.com/sdk/home#windows). It installs the
Vulkan loader, the validation layers, and the tools, and it sets the `VULKAN_SDK`
environment variable for you. Check it took with `echo %VULKAN_SDK%` in a new
terminal.

**Git** - if you don't have it, install
[Git for Windows](https://git-scm.com/download/win). During setup, the default
"Git from the command line and also from 3rd-party software" option is fine.

The validation layers come with the Vulkan SDK, so a normal SDK install covers
them. They matter because the default build is a debug build and the client
asks the loader for `VK_LAYER_KHRONOS_validation` at startup - without it the
client aborts on `vkCreateInstance`.

You don't have to open a Developer Command Prompt yourself. `build.bat` looks
for `vcvarsall.bat` (via `vswhere` and the usual install paths) and runs it for
you if `cl.exe` isn't already on the path. A regular Command Prompt or
PowerShell is fine.

## Getting the code

The dependencies (SDL3, ImGui, ImPlot, spdlog) are git submodules, so clone
recursively:

```cmd
git clone --recursive https://github.com/henri23/voltrum.git
cd voltrum
```

If you cloned without `--recursive`, or you pulled changes that moved a
submodule, sync them:

```cmd
git submodule update --init --recursive
```

Don't skip this after a `git pull`. Pulling updates the source but leaves
submodules where they were, so the ImGui checkout can lag behind what the
renderer expects and you'll see compile errors about missing members in the
Vulkan backend. The command above fixes that.

## Building

From the repository root:

```cmd
build.bat
```

It sets up MSVC, configures into `bin\`, builds the client, then builds and runs
the tests. It doesn't launch anything when it finishes.

The flags:

- `--dynamic` - build the core as a DLL instead of one static executable
- `--skip-tests` - don't build or run the tests
- `--asan` - compile with AddressSanitizer
- `--clean` - delete `bin\` before configuring

## Running

The build output lives under `bin\`:

```cmd
bin\client\voltrum_client.exe
```

A static build is self-contained. With `--dynamic` the client needs
`voltrum_core.dll` beside it, so run it from inside `bin\client`.

## When it doesn't work

**The build can't find a compiler.** Make sure the C++ workload is installed in
Visual Studio. If `build.bat` reports it can't locate `vcvarsall.bat`, your VS
install is incomplete or in an unusual location - open a "Developer Command
Prompt for VS" and run `build.bat` from there.

**`cmake` or `ninja` not recognized.** They aren't on your `PATH`. Reopen the
terminal after installing, or add their folders to `PATH`.

**The client aborts at startup with a validation-layer error.** The Vulkan SDK
isn't installed or `VULKAN_SDK` isn't set. Reinstall the SDK and check
`echo %VULKAN_SDK%` prints a path.

**Compile errors in `vulkan_ui_backend.cpp` about unknown ImGui members.** The
ImGui submodule is behind the code. Run
`git submodule update --init --recursive`.

For Linux and macOS see [BUILD_LINUX.md](BUILD_LINUX.md) and
[BUILD_MACOS.md](BUILD_MACOS.md).
