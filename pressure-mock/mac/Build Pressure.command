#!/bin/bash
# Double-click to build Mishkal Pressure (VST3 + AU + AAX + Standalone)
cd "$(dirname "$0")"
LOG="$PWD/Pressure-build.log"
exec > >(tee "$LOG") 2>&1
echo "=== Pressure build  $(date) ==="

if ! command -v cmake >/dev/null 2>&1; then
  if [ -x /Applications/CMake.app/Contents/bin/cmake ]; then
    export PATH="/Applications/CMake.app/Contents/bin:$PATH"
  else
    echo "CMake isn't installed. Get it from https://cmake.org/download (macOS .dmg), then double-click this file again."
    read -p "Press Return to close."; exit 1
  fi
fi
echo "CMake: $(cmake --version | head -1)"
echo "Xcode: $(xcodebuild -version | head -1)"

cd Pressure
echo "Setting up (uses Goliath's copy of JUCE)..."
FRESH=""; [ -f build/.configured ] || FRESH="--fresh"
cmake $FRESH -B build -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 && touch build/.configured || { echo; echo "SETUP FAILED - see Pressure-build.log on your Desktop"; read -p "Press Return to close."; exit 1; }
echo "Building (a few minutes the first time)..."
cmake --build build --config Release -- -quiet || { echo; echo "BUILD FAILED - see Pressure-build.log on your Desktop"; read -p "Press Return to close."; exit 1; }

echo
echo "=== BUILD SUCCEEDED ==="
ls -d ~/Library/Audio/Plug-Ins/VST3/"Mishkal Pressure.vst3" ~/Library/Audio/Plug-Ins/Components/"Mishkal Pressure.component" >/dev/null 2>&1 && echo "^ Pressure V$(grep -E '^set\(PRESSURE_LATEST_VERSION' CMakeLists.txt | grep -oE '[0-9]+' | head -1) installed (VST3 and AU) for Ableton."
ls -d build/AAX-ready-to-install/*.aaxplugin 2>/dev/null && echo "^ AAX ready for Pro Tools Developer."
read -p "Press Return to close."
