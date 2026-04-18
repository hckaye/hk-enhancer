# HK Enhancer - AI Agent Instructions

## Build Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --config Release
```

## Workflow: Implementation Checklist

After implementing any code change, complete ALL of the following before marking a task as done:

1. **Unit Test** - Write or update tests in `Tests/` for the changed code (if testable)
2. **Build** - `cmake --build build --config Release` passes with zero warnings in `Source/`
3. **Test** - `./build/HKEnhancerTests` passes all tests
4. **Lint** - `/opt/homebrew/opt/llvm/bin/clang-tidy --header-filter='Source/.*\.h$' -p build <changed files>` passes with zero warnings
5. **Format** - `/opt/homebrew/opt/llvm/bin/clang-format -i <changed files>` applied

Shorthand scripts:
- `./scripts/test.sh` - Build & run tests
- `./scripts/lint.sh` - Run clang-tidy on all sources
- `./scripts/format.sh` - Format all sources
- `./scripts/format.sh --check` - Check format without modifying

## Project Structure

- `Source/DSP/` - Audio processing (EnvelopeFollower, MultibandSplitter, TubeSaturator, HarmonicExciter, SubharmonicGenerator)
- `Source/GUI/` - UI components (LookAndFeel, BandControl)
- `Source/PluginProcessor.*` - JUCE AudioProcessor
- `Source/PluginEditor.*` - JUCE AudioProcessorEditor
- `Tests/` - Catch2 unit tests
