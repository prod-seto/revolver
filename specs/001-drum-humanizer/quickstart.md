# Quickstart: Drum Humanizer Dev Workflow

## Prerequisites

- macOS, CMake 3.22+, Xcode Command Line Tools, Node.js 18+
- No extra C++ dependencies (JUCE is fetched automatically)

## Dev Loop (browser UI, no plugin build required)

```bash
# Terminal 1 — C++ dev server (WebSocket on :9001)
./scripts/dev.sh

# Terminal 2 — Vite UI (hot-reload on :5173)
cd ui && npm install && npm run dev
```

Open `http://localhost:5173` in Chrome/Edge. The dev server handles:
- `fileDropped` → loads sample into AudioEngine
- `noteOn`/`noteOff` → plays sample via system audio output
- `generate` → triggers variation generation (placeholder until HumanizerEngine is shared)

To test drag-and-drop in dev mode: drag a WAV file onto the browser tab's drop zone.

## Plugin Build

```bash
./scripts/build.sh        # Builds VST3 + AU in build/
./scripts/install.sh      # Copies to ~/Library/Audio/Plug-Ins/
```

Open Logic Pro or Reaper, add "Revolver" as an instrument, load a project with MIDI.

## Running Tests

No automated test suite. Verify acceptance scenarios manually:

1. **Sample load**: Drag `samples/kick.wav` onto the plugin → confirm name + duration shown
2. **Generate**: Press Generate with chamberCount=4 → confirm 4 chambers active
3. **Playback**: Play MIDI notes → confirm each hit cycles through chambers (round-robin)
4. **State persist**: Save DAW project, close, reopen → confirm all parameters and sample restored

## Key Files

| File | Purpose |
|------|---------|
| `source/HumanizerEngine.h/.cpp` | Off-thread DSP engine, variation player |
| `source/PluginProcessor.h/.cpp` | APVTS, MIDI processBlock, message routing |
| `source/PluginEditor.h/.cpp` | FileDragAndDropTarget, WebBrowserComponent |
| `source/ServerMain.cpp` | Dev-mode message handlers (must mirror PluginProcessor) |
| `ui/main.js` | Cylinder UI, controls, drag-drop overlay |
| `ui/bridge.js` | Transport layer — do not add transport-specific code elsewhere |
| `specs/001-drum-humanizer/contracts/message-protocol.md` | Full message schema + parity checklist |

## Adding a New Message Type

1. Document the schema in `contracts/message-protocol.md`
2. Add handler to `ServerMain.cpp` `onMessage` lambda
3. Add handler to `PluginProcessor.cpp` `handleMessageFromUI`
4. Add UI handler via `webplug.on(type, fn)` in `ui/main.js`
5. Check the parity table in the contract as complete
