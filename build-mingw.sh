#!/usr/bin/env bash
# Cross-build of the Windows binaries (and the local-only web editor) on Linux + mingw-w64
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p dist
CXX=x86_64-w64-mingw32-g++
FLAGS="-O2 -fpermissive -w -msse4.1 -static -static-libgcc -static-libstdc++"
CORE="src/kraken_lib.cpp src/bitknit.cpp src/lzna.cpp"
x86_64-w64-mingw32-windres src/app.rc -O coff -o dist/app.res

$CXX $FLAGS -municode -mwindows -o dist/G1R-FixTool-GUI.exe src/gui.cpp $CORE dist/app.res \
  -lcomctl32 -lcomdlg32 -lshell32 -lgdi32
$CXX $FLAGS -municode -o dist/G1R-FixTool-Console.exe src/fixtool.cpp $CORE dist/app.res
$CXX $FLAGS -o dist/G1R-SaturasFix.exe src/patcher.cpp $CORE

cp fixes.ini dist/
# The web editor is local-only (web/ is gitignored) — build it only if present
if [ -f web/build_editor.py ]; then
  python3 web/build_editor.py && cp web/G1R-SaveEditor.html dist/
fi
rm -f dist/app.res
echo "dist/:" && ls -la dist/
