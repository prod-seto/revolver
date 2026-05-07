#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

SERVER="build/WebPlugServer_artefacts/Release/WebPlugServer"

if [ ! -f "$SERVER" ]; then
    echo "→ Server not built yet, building..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release --quiet
    cmake --build build --target WebPlugServer
fi

echo "→ Starting C++ DevServer on ws://localhost:9001"
exec "./$SERVER"
