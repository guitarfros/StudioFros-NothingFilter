# StudioFros - Nothing Filter

[![Build](https://github.com/OWNER/REPO/actions/workflows/build.yml/badge.svg)](https://github.com/OWNER/REPO/actions/workflows/build.yml)

A resonant band-pass "half-open wah" effect plugin (**VST3** for Linux, Windows and macOS) by **GuitarFros**.
Built with JUCE. Own DSP code, no third-party audio or samples.

> Replace `OWNER/REPO` in the badge above with your GitHub user and repository name.

## Controls
| Control | Type | Function |
|---|---|---|
| Position | knob | Pedal position (0-100 %, default 50 %) |
| Q / Peak | knob | Resonance of the filter |
| Range | knob | Upper sweep frequency (1.2-3.5 kHz) |
| Level | knob | Output level (-12/+12 dB) |
| Bypass | switch | Smooth bypass (LED dims when bypassed) |
| Mode | switch | **Fixed**: the Position knob controls the wah. **Pedal**: MIDI CC 11 or CC 4 controls it |
| Voicing | switch | Vintage (Q rises with position) / Modern (constant Q) |
| LCD | display | Shows values while you edit and the current preset name. Click it to open the preset list (Factory / User) |
| Left / right arrows | keys | Previous / next preset (wraps around: factory presets first, then your own) |
| SAVE / SAVE AS / DELETE / INIT | buttons | User presets / reset all. Factory presets are read-only: SAVE on one asks for a new name |

User presets are stored as XML in:
- Linux: `~/.config/GuitarFros/StudioFros-NothingFilter/Presets/`
- macOS: `~/Library/GuitarFros/StudioFros-NothingFilter/Presets/`
- Windows: `%APPDATA%\GuitarFros\StudioFros-NothingFilter\Presets\`

## Factory presets (18)
Half Open, Half Open Nasal, Low Quack, Cocked Mid, Vocal Open, Toe Down Bright, Heel Down Dark,
Funk Chop, Subtle Filter, Sharp Peak, Wide Sweep, Thin Treble, Modern Smooth, Pedal Control,
Pedal Modern, Rhythm Wah, Muted Half, Honk. They live in `Source/FactoryPresets.h`; add your own there.

## Download
Go to the **Releases** tab:
- **Latest build (main)**: rebuilt automatically on every push to `main` (pre-release, tag `nightly`).
- **vX.Y.Z**: versioned releases, created by pushing a tag (see below).

Each release has one zip per system (Linux, Windows, macOS universal) containing the `.vst3`.
Builds of every commit are also available in the **Actions** tab (artifacts).

## Build from source
Requirements: a C++17 compiler, CMake >= 3.22 and git. JUCE is found automatically (`./JUCE`, `../JUCE`,
`~/JUCE`, an installed package, or `-DJUCE_PATH=/path/to/JUCE`); if it is not found it is downloaded.

**Linux** (Ubuntu / Debian)

    sudo apt install build-essential cmake pkg-config libasound2-dev libx11-dev libxcomposite-dev \
      libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libfreetype-dev \
      libfontconfig1-dev libgl1-mesa-dev
    ./build.sh

**macOS** (Xcode command line tools + CMake)

    ./build.sh

Universal binary (Apple Silicon + Intel). The plugin is unsigned: if macOS blocks it, run
`xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/StudioFros - Nothing Filter.vst3"`.

**Windows** (Visual Studio 2022 or Build Tools + CMake; use a Developer Command Prompt)

    build.bat

Or by hand on any system:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel

## Install
| System | Folder |
|---|---|
| Linux | `~/.vst3/` (copied automatically after the build) |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` (copied automatically) |
| Windows | `C:\Program Files\Common Files\VST3\` (copy the `.vst3` folder by hand; administrator rights needed) |

Then re-scan plug-ins in your DAW (in Reaper: Options > Preferences > Plug-ins > VST > Re-scan).

## Images
The panel and knob artwork lives in `Assets/` and is embedded at build time. See `Assets/README.md`.

## Licence
- Copyright (C) 2026 GuitarFros. Code: AGPL-3.0-or-later (see `LICENSE`), because it links JUCE (AGPLv3).
  `Source/wah_core.h` is MIT (`LICENSES/MIT.txt`).
- Images in `Assets/`: generated with **Meta AI**; NOT AGPL. Distributed under Meta's terms of use for
  non-commercial purposes in this open-source project. See the additional notice at the end of `LICENSE`
  and `ASSETS_LICENSE.md`.
- "Meta" and "Meta AI" are trademarks of Meta Platforms, Inc. VST is a trademark of Steinberg Media
  Technologies GmbH. See `NOTICE.md`. This project is not affiliated with or endorsed by any third party.

## Publishing checklist
1. Create the GitHub repository and push this folder.
2. Replace `OWNER/REPO` in the badge at the top of this README.
3. Check the **Actions** tab: Linux, Windows and macOS builds should be green.
4. Every push to `main` updates the **Latest build (main)** release automatically. For a versioned release, create a tag: `git tag v0.1.0 && git push --tags` (zips for all three systems are attached automatically).
