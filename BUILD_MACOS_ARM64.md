# Building ep128emu on Apple Silicon

This branch builds ep128emu 2.0.11.2 as a native macOS ARM64 application.

## Requirements

- Apple Silicon Mac
- Xcode Command Line Tools
- Homebrew
- CMake
- SCons
- pkg-config
- Homebrew packages: `portaudio`, `libsndfile`, `sdl12-compat`, `sdl2-compat`, `sdl3`, `lua`

The versions used by the reference build are listed in `DEPENDENCY_MANIFEST.txt`.

FLTK is intentionally not taken from Homebrew. `REBUILD_MACOS_ARM64.sh`
downloads the official FLTK 1.4.5 source archive, verifies its SHA-256,
and builds a private ARM64 FLTK prefix including `fluid`.

## One-command rebuild

```sh
./REBUILD_MACOS_ARM64.sh
```

The script performs a clean source build, bundles all non-system dynamic
libraries into the application, applies ad-hoc signing, verifies the
bundle, and writes the final app and ZIP to `rebuild-output/`.

Developer ID signing and Apple notarization are not included.
