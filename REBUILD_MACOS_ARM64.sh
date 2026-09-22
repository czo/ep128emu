#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$ROOT"

for c in brew cmake scons pkg-config curl shasum ditto codesign xcrun; do
  command -v "$c" >/dev/null 2>&1 || { echo "Missing required command: $c" >&2; exit 1; }
done
for p in portaudio libsndfile sdl12-compat sdl2-compat sdl3 lua; do
  brew --prefix "$p" >/dev/null 2>&1 || { echo "Missing Homebrew package: $p" >&2; exit 1; }
done

DEPS="$ROOT/.deps"
TAR="$DEPS/fltk-1.4.5-source.tar.gz"
URL='https://github.com/fltk/fltk/releases/download/release-1.4.5/fltk-1.4.5-source.tar.gz'
EXPECTED=eede1fb2b8e9c2e581e77082e15252145855c79aad30070ee3b24aabe2f926f1
mkdir -p "$DEPS"
if [ ! -f "$TAR" ]; then
  echo 'Downloading FLTK 1.4.5 source...'
  curl -fL "$URL" -o "$TAR"
fi
ACTUAL="$(shasum -a 256 "$TAR" | awk '{print $1}')"
[ "$ACTUAL" = "$EXPECTED" ] || { echo "FLTK source SHA-256 mismatch" >&2; exit 1; }

FLTK_SRC="$DEPS/fltk-1.4.5-src"
FLTK_BLD="$DEPS/fltk-1.4.5-build"
FLTK_PFX="$DEPS/fltk-1.4.5"
rm -rf "$FLTK_SRC" "$FLTK_BLD" "$FLTK_PFX"
mkdir -p "$FLTK_SRC" "$FLTK_BLD"
tar -xzf "$TAR" -C "$FLTK_SRC" --strip-components=1
cmake -S "$FLTK_SRC" -B "$FLTK_BLD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_INSTALL_PREFIX="$FLTK_PFX" \
  -DFLTK_BUILD_SHARED_LIBS=ON \
  -DFLTK_BUILD_TEST=OFF \
  -DFLTK_BUILD_EXAMPLES=OFF \
  -DFLTK_BUILD_FLUID=ON \
  -DFLTK_BUILD_FORMS=OFF
cmake --build "$FLTK_BLD" -j"$(sysctl -n hw.logicalcpu)"
cmake --install "$FLTK_BLD"

export EP128EMU_FLTK_PREFIX="$FLTK_PFX"
export PATH="$ROOT/tools:$FLTK_PFX/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin"

scons -c >/dev/null 2>&1 || true
rm -rf .sconf_temp .sconsign.dblite config.log
rm -f ep128emu epmakecfg tapeedit *.a
find . -name '*.o' -type f -delete
rm -rf ep128emu.app/Contents/MacOS ep128emu.app/Contents/Frameworks
scons -j"$(sysctl -n hw.logicalcpu)" utils=0

OUT="$ROOT/rebuild-output"
APP="$OUT/ep128emu-2.0.11.2-macOS-ARM64-2026.09.app"
ZIP="$OUT/ep128emu-2.0.11.2-macOS-ARM64-2026.09.zip"
rm -rf "$OUT"
mkdir -p "$OUT"
ditto ep128emu.app "$APP"
EP128EMU_FLTK_PREFIX="$FLTK_PFX" ./tools/bundle-macos-deps.sh "$APP"
codesign --verify --deep --strict "$APP"
ditto -c -k --norsrc --noextattr --keepParent "$APP" "$ZIP"

echo "APP SHA-256: $(shasum -a 256 "$APP/Contents/MacOS/ep128emu" | awk '{print $1}')"
echo "ZIP SHA-256: $(shasum -a 256 "$ZIP" | awk '{print $1}')"
echo '=== REBUILD COMPLETE: PASS ==='
