# LiveCut Enhanced

A modern 64-bit Windows build of **Livecut**, Remy Muller's (mdsp) classic
beat-slicer from 2004, based on the BBCut algorithm (Nick Collins). This project
combines the original DSP core with a new GUI built in JUCE 8.

The GUI has a help mode: click the **?** in the top-right corner, then hover
over any control for an explanation of what the parameter does.

**Before:** Livecut only started a new rhythmic "phrase" on a bar line. Loop a region shorter than a bar, or one that didn't contain a downbeat, and the engine could get stuck, going silent (or never starting) until you stopped and restarted playback.

**The fix:** The engine now detects the playhead jump that happens at a loop's wrap point and uses that as the cue to start a fresh phrase immediately, downbeat or not.

**What it means for you:** Short loops keep cutting indefinitely, off-downbeat loops actually produce sound, and you never need to stop/restart playback to "wake" the plugin.

## Formats

- VST3 (Windows x64, macOS universal: arm64 + x86_64)
- Standalone app (for quick testing)

## Structure

| Directory                      | Contents                                                                                                   |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------- |
| `src/dsp/`                     | DSP core (BBCutter, BitCrusher, Comb, etc.), GPL code from the original, with `float_cast.h` fixed for x64 |
| `src/`                         | JUCE processor, parameters (APVTS) and editor                                                              |
| `src/ui/`                      | LookAndFeel for the modern dark theme                                                                      |
| `tests/`                       | Host-simulation regression tests for the DSP kernel (CTest)                                                |
| `vendor/JUCE`                  | JUCE 8 (cloned, not tracked in this repo)                                                                  |
| `third_party/livecut-upstream` | scheffle/Livecut (reference; the DSP source was copied from here)                                          |

## Building

JUCE is not tracked in this repo, so clone it into `vendor/JUCE` first (same
step on every platform):

```sh
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE.git vendor/JUCE
```

### Windows

Requires Visual Studio 2026 (or 2022) with the C++ workload and CMake 3.22+.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

Artifacts end up in `build/LiveCutVariant_artefacts/Release/`:

- `VST3/LiveCut Enhanced.vst3` - copy to `C:\Program Files\Common Files\VST3\`
- `Standalone/LiveCut Enhanced.exe`

### macOS

Requires the Xcode Command Line Tools (`xcode-select --install`) and CMake
3.22+. Full Xcode is only needed if you also want the Audio Unit (AU) format.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

Building on an Apple Silicon Mac produces a native arm64 binary; building on an
Intel Mac produces x86_64. To produce a single universal binary that runs
natively on both (no Rosetta), add the architectures flag when configuring:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

Artifacts end up in `build/LiveCutVariant_artefacts/Release/`:

- `VST3/LiveCut Enhanced.vst3` - copy to `~/Library/Audio/Plug-Ins/VST3/`
- `Standalone/LiveCut Enhanced.app`

Verify the built architectures with `lipo -info "build/LiveCutVariant_artefacts/Release/VST3/LiveCut Enhanced.vst3/Contents/MacOS/LiveCut Enhanced"`.

## Testing

```sh
ctest --test-dir build -C Release
```

`tests/KernelSilenceTest.cpp` drives the DSP kernel the way a host would
(arrangement loops with buffer splits at the loop wrap, block sizes aligned to
bar boundaries, loops shorter than one bar) and fails if the cutter ever stops
producing output.

## Provenance and license

- Original: [mdsp @ smartelectronix](https://github.com/mdsp/Livecut), GPL v2+
- VST3 adaptation of the DSP core: [scheffle/Livecut](https://github.com/scheffle/Livecut)
- Community hub: [mrbfrank/Livecut-64bit](https://github.com/mrbfrank/Livecut-64bit)

This project is licensed under **GPL v2 or later**, same as the original.

## Usage

LiveCut Enhanced is an effect that slices incoming audio in real time, synced to the
host transport/tempo. Three cut procedures:

- **CutProc11**: classic slicing with repeats and stutter
- **WarpCut**: accelerating/ritarding repeats
- **SQPusher**: patterns inspired by Squarepusher fills

The bitcrusher and comb filter are applied per cut. In
standalone mode (without a host transport) the plugin runs against an internal
clock at 120 BPM.
