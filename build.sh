#!/bin/sh
# Build PCMigrate.exe on Linux (or in GitHub Actions) with mingw-w64:
#   sudo apt install g++-mingw-w64-x86-64
set -e
cd "$(dirname "$0")"
mkdir -p build
x86_64-w64-mingw32-windres src/app.rc -O coff -o build/app.res
x86_64-w64-mingw32-g++ -std=c++17 -O2 -s -municode -mwindows -static -static-libgcc -static-libstdc++ \
  src/main.cpp build/app.res -o build/PCMigrate.exe \
  -lcomctl32 -lshell32 -lole32 -luuid -ladvapi32 -lshlwapi -luser32 -lgdi32
echo "OK -> build/PCMigrate.exe"
