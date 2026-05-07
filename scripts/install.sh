#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

echo "→ Building plugin..."
cmake --build build --target WebPlugPlugin_VST3 WebPlugPlugin_AU

VST3_SRC="build/WebPlugPlugin_artefacts/Release/VST3/WebPlug.vst3"
VST3_DST="$HOME/Library/Audio/Plug-Ins/VST3/WebPlug.vst3"
AU_SRC="build/WebPlugPlugin_artefacts/Release/AU/WebPlug.component"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components/WebPlug.component"

if [ -d "$VST3_SRC" ]; then
    echo "→ Installing VST3..."
    rm -rf "$VST3_DST"
    cp -r "$VST3_SRC" "$VST3_DST"
    echo "✓ $VST3_DST"
fi

if [ -d "$AU_SRC" ]; then
    echo "→ Installing AU..."
    rm -rf "$AU_DST"
    cp -r "$AU_SRC" "$AU_DST"
    echo "✓ $AU_DST"
fi

echo ""
echo "✓ Done. Rescan plugins in your DAW."
