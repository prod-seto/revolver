#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR/ui"

if [ ! -d node_modules ]; then
    echo "→ Installing npm dependencies..."
    npm install
fi

echo "→ Starting Vite dev server on http://localhost:5173"
npm run dev
