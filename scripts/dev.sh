#!/bin/bash
# Opens the full dev environment: C++ server + Vite UI in two Terminal windows.
set -e
DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "→ Launching dev environment..."

osascript <<APPLE
tell application "Terminal"
    activate
    do script "echo '▶  WebPlug — C++ DevServer' && '$DIR/scripts/server.sh'"
end tell
APPLE

sleep 0.3

osascript <<APPLE
tell application "Terminal"
    activate
    do script "echo '▶  WebPlug — Vite UI' && '$DIR/scripts/ui.sh'"
end tell
APPLE

echo "✓ Two Terminal windows opened"
echo "  Once both are running, open http://localhost:5173"
