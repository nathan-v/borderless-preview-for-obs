# Contributing

## Code Style

clang-format formats the C and C++ sources and gersemi formats the CMake files. Both checks run in CI. The clang-format version is pinned to 19.1.x by `build-aux/run-clang-format`, matching obs-plugintemplate; newer or older releases are rejected.

```bash
./build-aux/run-clang-format --check --fail-error
./build-aux/run-gersemi --check --fail-error
```

Drop `--check` to apply the formatting in place.

## Toolchain Versions

C++17 throughout. OBS Studio 30 or newer at runtime; the build pins the obs-studio 31.1.1 sources plus the matching obs-deps and Qt 6 bundles in `buildspec.json`. CMake 3.28 or newer. Windows builds need Visual Studio 2022, macOS builds need Xcode 16 with the macOS 15 SDK, and Ubuntu builds need the apt packages listed in the README.

## Developer Setup

```bash
git clone https://github.com/nathan-v/borderless-preview-for-obs.git
cd borderless-preview-for-obs
cmake --preset macos          # or windows-x64, ubuntu-x86_64
cmake --build --preset macos --config RelWithDebInfo
```

The Windows and macOS presets download the pinned OBS and Qt dependencies into `.deps/` on first configure. The Ubuntu preset links against the system OBS, so `libobs-dev` must match the OBS you run.

## Testing

The unit tests cover the parts that run without OBS: the letterbox fit math and the central-widget swap. They use a small self-contained harness in `tests/check.hpp` and a headless Qt platform (offscreen, or minimal on Windows), so they need no display. Build them on their own or alongside the plugin:

```bash
cmake -S tests -B build_tests -DCMAKE_BUILD_TYPE=Debug   # standalone; needs Qt 6 Widgets on CMAKE_PREFIX_PATH
cmake --build build_tests
ctest --test-dir build_tests --output-on-failure
```

```bash
cmake --preset macos -DENABLE_TESTS=ON                    # or windows-x64, ubuntu-x86_64
cmake --build --preset macos --config RelWithDebInfo
ctest --preset macos
```

CI runs the standalone form on Ubuntu, macOS and Windows; the latter two use the same Qt bundle the plugin ships with. Add a test for any change to `canvas-fit.hpp` or `central-swap.cpp`; a bug fix there should come with a regression case.

Everything that touches libobs (the display surface, the draw callback, hotkeys, settings) is verified by running the plugin in OBS. Before opening a PR, install your build and check:

- Toggle on and off from the Tools menu, the hotkey, and a double-click; the stock preview comes back intact each time.
- Studio Mode on and off while the borderless view is active.
- Scene and scene collection switches while the view is active.
- Quit OBS with the view active; on relaunch it comes back on and the window layout is unchanged.

Test on more than one platform if you can; the display surface code has per-platform branches.

## Pull Request Process

1. Make sure the clang-format and gersemi checks pass
2. Make sure the unit tests and the CI build on all three platforms pass
3. Update the README if your change affects usage or configuration
4. Reference any related issues in your PR description
