# webplug

Web-first JUCE plugin scaffold. Develop your plugin UI in the browser with hot-reload, then ship it as a VST3/AU. Same code, two transports.

## How it works

There are two ways to run the plugin:

**Dev mode:** the C++ audio engine runs as a local WebSocket server. Your browser connects to it and serves as the UI with Vite hot-reload. Change any HTML/CSS/JS and the browser updates instantly with no rebuild.

**Plugin mode:** the same UI is embedded in a VST3/AU plugin via JUCE's WebBrowserComponent. The WebSocket transport is swapped out for JUCE's native function bridge. No UI code changes required.

```
Dev mode:     Browser <-- WebSocket --> C++ DevServer
Plugin mode:  WebView <-- __juce__  --> C++ AudioProcessor
```

The `bridge.js` file auto-detects which transport to use at runtime.

## Prerequisites

- macOS (Windows support is possible but untested)
- CMake 3.22+
- Xcode Command Line Tools
- Node.js 18+

## Getting started

```bash
git clone <repo-url>
cd webplug

# Build everything (first time takes a few minutes, downloads JUCE via FetchContent)
./scripts/build.sh
```

## Dev workflow

```bash
./scripts/dev.sh
```

This opens two Terminal windows: the C++ server on `ws://localhost:9001` and the Vite dev server on `http://localhost:5173`. Open the browser URL and click **PING C++** to verify the round trip.

The C++ server only needs to be restarted when you change C++ code. UI changes (HTML/CSS/JS) hot-reload automatically.

## Install to DAW

```bash
./scripts/install.sh
```

Builds the VST3 and AU and copies them to:
- `~/Library/Audio/Plug-Ins/VST3/`
- `~/Library/Audio/Plug-Ins/Components/`

Rescan plugins in your DAW after running this.

## Project structure

```
source/
  PluginProcessor.h/cpp   # AudioProcessor, handles C++ <-> UI message routing
  PluginEditor.h/cpp      # Hosts the WebBrowserComponent, writes UI to temp dir
  DevServer.h/cpp         # WebSocket server (RFC 6455, no external deps)
  ServerMain.cpp          # Entry point for the standalone dev server binary

ui/
  index.html              # Plugin shell
  bridge.js               # Transport layer (auto-selects WebSocket or __juce__)
  main.js                 # UI logic; replace this with your plugin's code
  styles.css              # Styles
  package.json / vite.config.js

scripts/
  build.sh                # Configure and build all targets
  dev.sh                  # Open dev environment (server + UI) in two terminals
  install.sh              # Build and install VST3 + AU
  server.sh               # Start just the C++ server
```

## Message protocol

All messages between the UI and C++ are JSON:

```json
{ "type": "ping", "payload": { "message": "hello" } }
```

**UI to C++:** call `webplug.send(type, payload)` from any JS file.  
**C++ to UI:** call `onMessageToUI(json)` from the processor (plugin) or `server.sendMessage(json)` from the dev server.

The scaffold includes a `ping` / `pong` round trip as a proof of life. Add your own message types in `ServerMain.cpp` / `PluginProcessor.cpp` and handle them in `main.js`.

## Building your own plugin on top of this

1. Clone or copy this repo
2. Rename `WebPlug` to your plugin name throughout `CMakeLists.txt`
3. Add your DSP code to `PluginProcessor.cpp`
4. Add message handlers to both `PluginProcessor.cpp` (plugin) and `ServerMain.cpp` (dev server), keeping them in sync
5. Replace `ui/main.js` with your UI logic
6. Use `./scripts/dev.sh` to iterate and `./scripts/install.sh` to test in a DAW
