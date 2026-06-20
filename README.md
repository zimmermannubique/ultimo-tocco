# L'Ultimo Tocco — di Maestro Marco

> *the last plugin in your chain…*

A one-knob "vibe corrector" audio plugin built with [JUCE](https://juce.com).
Turn the knob: the portrait rises to its happiest around ~30% of the travel, then
slowly slides into doubt — while the output level barely nudges (0 dB through the
early range, up to +0.25 dB at the very end). It's mostly placebo, by design.

Built as AU / VST3 / Standalone.

## Building locally

Requires CMake ≥ 3.22 and a C++17 toolchain. JUCE is pulled in automatically:
if a `JUCE/` checkout sits next to this folder it is used, otherwise the matching
version (8.0.13) is fetched from GitHub.

```bash
cmake -S . -B build
cmake --build build --config Release --parallel
```

Artefacts land in `build/MoodMixer_artefacts/Release/{AU,VST3,Standalone}/`.

### Install for Logic (macOS)

```bash
cp -R "build/MoodMixer_artefacts/Release/AU/Ultimo Tocco.component" \
   ~/Library/Audio/Plug-Ins/Components/
```

Then fully relaunch Logic (Audio Units are scanned at startup).

## Continuous builds

`.github/workflows/build.yml` builds the plugin on every push:

- **Windows** (`windows-latest`, MSVC) → VST3 + Standalone
- **macOS** (`macos-latest`, Xcode) → AU + VST3 + Standalone

You do **not** need a Windows machine — the Windows runner produces the
Windows VST3. Download the results from the run's **Artifacts** section.
