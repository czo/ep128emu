#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
FLTK_PREFIX="${EP128EMU_FLTK_PREFIX:-/opt/homebrew}"
export PATH="$ROOT/tools:$FLTK_PREFIX/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin"
scons -c >/dev/null 2>&1 || true
rm -rf .sconf_temp .sconsign.dblite config.log
rm -f ep128emu epmakecfg tapeedit
rm -rf ep128emu.app/Contents/MacOS ep128emu.app/Contents/Frameworks
scons -j"$(sysctl -n hw.logicalcpu)" utils=0
