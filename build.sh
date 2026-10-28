#!/usr/bin/env bash
# Linux / macOS.  Usage: ./build.sh [-DJUCE_PATH=/path/to/JUCE]
set -e
cd "$(dirname "$0")"
touch CMakeLists.txt Source/*.cpp Source/*.h
cmake -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build --config Release --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
echo
echo "Built:"; find build -maxdepth 4 -name "*.vst3" -type d
case "$(uname -s)" in
  Darwin) echo "Installed in: ~/Library/Audio/Plug-Ins/VST3" ;;
  *)      echo "Installed in: ~/.vst3" ;;
esac
