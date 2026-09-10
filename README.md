# Borderless Preview for OBS

[![GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-blue.svg)](https://github.com/nathan-v/borderless-preview-for-obs/blob/main/LICENSE)
[![OBS Studio 30+](https://img.shields.io/badge/OBS_Studio-30%2B-blue.svg)](https://obsproject.com/)
[![CI](https://github.com/nathan-v/borderless-preview-for-obs/actions/workflows/ci.yml/badge.svg)](https://github.com/nathan-v/borderless-preview-for-obs/actions/workflows/ci.yml)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/nathan-v/borderless-preview-for-obs)](https://github.com/nathan-v/borderless-preview-for-obs/releases)
[![GitHub last commit](https://img.shields.io/github/last-commit/nathan-v/borderless-preview-for-obs)](https://github.com/nathan-v/borderless-preview-for-obs/commits/main)

Give the whole preview to the picture. Borderless Preview draws your canvas edge to edge, with none of the border, label row or zoom bar that OBS wraps around it.

OBS's preview gives up pixels on every side. On a wide desktop monitor you hardly notice; on a portrait 1080x1920 screen the chrome takes a good slice of the height, and the picture you are trying to watch shrinks to fit what is left. This plugin lifts OBS's own preview out of the window and puts a bare display surface in its place. Toggle it off and the stock preview is back exactly as it was, docks and all.

> **Look, don't touch.** The borderless view is for watching. To select, move or resize a source, toggle back to the normal preview; that is one double-click away and nothing changes in between.

If you have questions or want to talk about this plugin you can find me on [Twitch](https://twitch.tv/The_Nathan_V).

## Contents

- [What it does](#what-it-does)
- [Getting started](#getting-started)
- [How you would actually use it](#how-you-would-actually-use-it)
- [Staying safe](#staying-safe)
- [Status](#status)
- [For developers](#for-developers)
- [License](#license)

## What it does

- **Edge to edge.** The picture fills the preview area right up to the letterbox. The 10 px border, the row above and the zoom bar below are gone while it is on.
- **Follows Studio Mode.** Shows the scene you are preparing while Studio Mode is on, and the live program otherwise.
- **Swaps in place.** Turn it off and the stock preview comes back exactly as it was. Your docks never move.
- **Remembers your choice.** Turn it on once and it is on the next time OBS starts.
- **Never phones home.** No analytics, no tracking, no update checks; this plugin does _not_ call home in any way.

## Getting started

You need OBS Studio 30 or newer. Grab the build for your computer from the [releases page](https://github.com/nathan-v/borderless-preview-for-obs/releases) and install it:

| Platform | How |
| --- | --- |
| Windows | Run `borderless-preview-for-obs-<version>-windows-x64-installer.exe`. It asks you to close OBS first and adds an uninstaller to Windows Settings. |
| macOS | Open the `.pkg`, or copy `borderless-preview-for-obs.plugin` into `~/Library/Application Support/obs-studio/plugins/`. |
| Linux | Install the `.deb` on Ubuntu, or put `borderless-preview-for-obs.so` in `~/.config/obs-studio/plugins/borderless-preview-for-obs/bin/64bit/` and `en-US.ini` in `~/.config/obs-studio/plugins/borderless-preview-for-obs/data/locale/`. |

On macOS the `.pkg` and the plugin inside it are signed only with an ad-hoc signature and are not notarized, so Gatekeeper will say the package is from an unidentified developer. Control-click the `.pkg` and choose Open, or allow it under System Settings → Privacy & Security, after checking its checksum.

Restart OBS, then turn it on from **Tools → Borderless preview**. Every release lists SHA-256 checksums; check your download against them before installing.

## How you would actually use it

**A clean output monitor.** Drag the OBS window onto a spare screen, hide the docks you do not need (View → Docks), and turn the borderless preview on. What is left is exactly what your viewers see, at the largest size that screen can give it.

**A portrait second screen.** Put the OBS window on that monitor with your docks below the preview, and let the picture use the full width. This is the setup the plugin was written for. Even on a 3440-wide window the built-in preview loses about 45 px of width and 77 px of height to chrome; a portrait screen has none to spare.

**Going back to edit.** Double-click the picture and the normal preview returns with its red outlines and handles. Move your source, then turn the borderless view back on from the Tools menu. If you toggle a lot, give it a key under Settings → Hotkeys → "Borderless Preview: toggle"; it works from a Stream Deck like any other OBS hotkey.

**With Studio Mode.** Studio Mode puts two views side by side, the scene you are preparing and the one that is live. The borderless view shows only the one you are preparing, full size. To see both again, toggle back.

The three ways to toggle, for reference:

| Toggle | Where |
| --- | --- |
| Menu | Tools → Borderless preview |
| Hotkey | Settings → Hotkeys → "Borderless Preview: toggle" (unbound until you pick a key) |
| Double-click | Left double-click the borderless view to switch back to the stock preview |

There is one setting and the plugin writes it for you: whether the borderless view is on when OBS starts. It lives in `config.json` under OBS's `plugin_config/borderless-preview-for-obs/` folder, should you ever want to reset it by hand.

## Staying safe

- **It runs inside OBS.** Like every OBS plugin it loads into the OBS process with the same access OBS has. Install builds from the releases page and check the checksums, or build it yourself.
- **One file, no network.** The plugin reads and writes its own `config.json` and nothing else, and never opens a connection.
- **Your scenes are untouched.** The stock preview is only hidden while the borderless view is on; toggling off restores it as it was.

See [SECURITY.md](SECURITY.md) to report a problem.

## Status

Working. Verified by hand in OBS on macOS and Windows. The Linux build passes CI along with the unit tests but has had less time in front of a real OBS. On Linux the view works on X11 and XWayland; native Wayland needs a build against Qt 6.9 or newer, which the release builds are not.

## For developers

Built on the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate); CMake 3.28+.

```bash
cmake --preset macos                                   # or windows-x64, ubuntu-x86_64; first run downloads OBS and Qt
cmake --build --preset macos --config RelWithDebInfo
cmake --install build_macos --config RelWithDebInfo --prefix release
```

Ubuntu wants `build-essential cmake ninja-build pkg-config libobs-dev obs-studio qt6-base-dev qt6-base-private-dev` from apt first. The Windows installer needs [Inno Setup 6](https://jrsoftware.org/isinfo.php); `pwsh ./installer/windows/build-installer.ps1`.

Unit tests cover what runs without OBS, the letterbox fit math and the central-widget swap, and need only a C++17 compiler and Qt 6 Widgets:

```bash
cmake -S tests -B build_tests && cmake --build build_tests && ctest --test-dir build_tests
```

Format check before pushing; `./build-aux/run-clang-format --check` (clang-format 19.1.x) and `./build-aux/run-gersemi --check`.

`.github/workflows/ci.yml` runs both format checks, the unit tests on Ubuntu, macOS and Windows, and builds all three platforms with the Windows installer on every push and pull request. A `x.y.z` tag creates a draft release with checksums. [CONTRIBUTING.md](CONTRIBUTING.md) has the workflow and the by-hand checklist for the parts that touch libobs.

## License

GPL-2.0-or-later; see [LICENSE](LICENSE). The display surface and render path follow OBS Studio's `OBSQTDisplay` and `OBSBasic_Preview.cpp` (Copyright (C) 2023 Lain Bailey, GPL v2+). PRs and constructive feedback are welcome.
