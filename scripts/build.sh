#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

echo "→ Configuring..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "→ Building all targets..."
cmake --build build

echo ""
echo "✓ Build complete"
echo "  Server:  build/WebPlugServer_artefacts/Release/WebPlugServer"
echo "  Plugin:  build/WebPlugPlugin_artefacts/Release/VST3/WebPlug.vst3"
echo "           build/WebPlugPlugin_artefacts/Release/AU/WebPlug.component"
