# Implementation Plan: Drum Humanizer (001)

**Branch**: `001-drum-humanizer` | **Date**: 2026-05-06 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-drum-humanizer/spec.md`

## Summary

Build a drum humanizer JUCE plugin on the WebPlug scaffold: drag-and-drop WAV/AIF loading,
off-thread generation of 4/6/8 variation buffers (pitch ± cents, gain ± dB, HF rolloff),
MIDI-triggered round-robin/random playback with velocity scaling, and a cylinder UI
with per-chamber animation. All three DAW parameters (variationAmount, chamberCount,
playbackMode) are automatable via APVTS; full state persists across DAW project saves.

## Technical Context

**Language/Version**: C++17 (JUCE, FetchContent) + JavaScript ES5 (no framework, Vite dev)
**Primary Dependencies**: JUCE (sole C++ dep per Constitution IV), Vite (dev server only)
**Storage**: JUCE `getStateInformation`/`setStateInformation` — APVTS XML + sample path string
**Testing**: Manual dev-loop verification (`./scripts/dev.sh`) + DAW load test (Logic Pro / Reaper)
**Target Platform**: macOS VST3 + AU (JUCE `juce_add_plugin`); dev mode Chromium
**Project Type**: Audio plugin (JUCE) + standalone WebSocket dev server
**Performance Goals**:
- SC-001: Generate 4 variations from a <500ms sample in under 3s
- SC-003/004: No added MIDI→audio latency; zero dropped notes at <100 note-ons/sec
- SC-005: `chamberFired` message delivered within one animation frame (~16ms) of note-on
**Constraints**:
- Audio thread: zero allocation, no blocking (FR-017)
- All DSP runs off-thread in a background `juce::Thread`
- No external C++ libraries beyond JUCE (Constitution IV)
**Scale/Scope**: 1 sample, up to 8 pre-rendered variation buffers, 1 plugin instance

## Constitution Check

*GATE: Must pass before implementation. Re-checked post-design below.*

### I. Dual-Transport Unity ✅
All new message types (`fileDropped`, `generate`, `setParameter`, `stateSync`,
`sampleLoaded`, `sampleError`, `generationStarted`, `generationComplete`, `chamberFired`,
`parameterChanged`) MUST be handled in both `ServerMain.cpp` (dev) and
`PluginProcessor.cpp` (plugin). See `contracts/message-protocol.md` parity checklist.

### II. Web-First Development ✅
All UI work (cylinder component, drag-drop overlay, controls) is done in `ui/` and
verified in the browser via `./scripts/dev.sh` before any plugin build.

### III. Message Protocol Consistency ✅
New message types are documented in `contracts/message-protocol.md` with schemas before
implementation. Parity checklist in that file must be fully checked before merge.

### IV. Minimal C++ Dependencies ✅ (no violation)
DSP uses only JUCE primitives:
- Pitch shift: `juce::LagrangeInterpolator`
- HF rolloff: `juce::IIRFilter` + `juce::IIRCoefficients::makeLowPass`
- Gain: `juce::AudioBuffer::applyGain`
No external C++ library introduced.

### V. Scaffold Integrity ✅
`./scripts/build.sh` and `./scripts/install.sh` must succeed after:
- Adding `NEEDS_MIDI_INPUT TRUE` + `IS_SYNTH TRUE` to `CMakeLists.txt`
- Adding APVTS members to PluginProcessor
- Adding `juce_audio_dsp` to plugin `target_link_libraries` (needed for IIRCoefficients
  — must be verified; `juce::IIRFilter` may be in `juce_audio_basics` already)

**Post-design gate re-check**: No constitution violations. Complexity table not required.

## Project Structure

### Documentation (this feature)

```text
specs/001-drum-humanizer/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── contracts/
│   └── message-protocol.md  # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
source/
├── PluginProcessor.h          # Add APVTS, VariationSet, HumanizerEngine
├── PluginProcessor.cpp        # Wire APVTS, MIDI processBlock, message handlers
├── PluginEditor.h             # Add FileDragAndDropTarget
├── PluginEditor.cpp           # Add filesDropped, isInterestedInFileDrag
├── HumanizerEngine.h          # NEW: off-thread generation + variation playback
├── HumanizerEngine.cpp        # NEW: LagrangeInterpolator DSP, IIRFilter, VariationSet
├── DevServer.h                # Unchanged
├── DevServer.cpp              # Unchanged
└── ServerMain.cpp             # Add handlers: fileDropped, generate, setParameter

ui/
├── index.html                 # Replace scaffold UI with drum humanizer UI
├── main.js                    # Replace with cylinder, controls, drag-drop overlay
├── styles.css                 # Replace with drum humanizer styles
├── bridge.js                  # Unchanged (transport-agnostic)
└── keyboard.js                # Keep (useful for dev-mode note triggering)

CMakeLists.txt                 # NEEDS_MIDI_INPUT TRUE, IS_SYNTH TRUE, juce_audio_dsp
```

**Structure Decision**: Single-project layout (existing scaffold). New C++ code goes into
`HumanizerEngine` to keep the scaffold files clean and the feature separable.

## Complexity Tracking

> No constitution violations — this table is empty.

---

## Implementation Phases (for /speckit-tasks)

### Layer 0: C++ Infrastructure

**0.1 — HumanizerEngine (core)**

New class `HumanizerEngine` (owns `VariationSet`, background generation thread, playback
state). Lives in `source/HumanizerEngine.h/.cpp`.

Key responsibilities:
- `loadSample(juce::File)` → validates, loads into `sampleBuffer`, notifies listener
- `generateVariations(int chamberCount, float variationAmount)` → spawns background
  `juce::Thread`, renders buffers, atomically swaps `currentSet`
- `getNextVariation(float velocity)` → called from audio thread; returns pointer to
  variation buffer + gain; advances round-robin index
- `processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&)` → reads MIDI events,
  mixes active voices into output buffer

**DSP per variation** (off-thread):
1. Copy source `sampleBuffer` to variation buffer (resample to host sample rate if needed)
2. Pitch shift: `LagrangeInterpolator` two-pass (±5 cents max × variationAmount, random)
3. Gain: `applyGain(dB)` (±1 dB max × variationAmount, random)
4. HF rolloff: `juce::IIRFilter::makeLowPass` (8kHz–20kHz × variationAmount, random)

**Voice model** (audio thread):
- Simple one-shot polyphonic: up to 8 simultaneous voices (one per MIDI note-on)
- Each voice tracks: `variationIndex`, `readPos`, `gainScale`
- Voice allocation: steal oldest if all 8 slots occupied

**0.2 — APVTS in PluginProcessor**

Wire `AudioProcessorValueTreeState` with three parameters (R-003). Implement
`getStateInformation`/`setStateInformation` + `stateSync` dispatch.

**0.3 — CMakeLists.txt changes**

```cmake
NEEDS_MIDI_INPUT  TRUE
IS_SYNTH          TRUE
```

Add `juce::juce_audio_dsp` to plugin link libraries (for IIRCoefficients if not
already in audio_basics).

**0.4 — FileDragAndDropTarget on PluginEditor**

Inherit `juce::FileDragAndDropTarget`, implement `isInterestedInFileDrag` and
`filesDropped` (sends `fileDropped` JSON to `handleMessageFromUI`).

**0.5 — Message handlers (PluginProcessor + ServerMain)**

Add symmetrical handlers for all new message types per `contracts/message-protocol.md`.
PluginProcessor also implements `AudioProcessorListener` or polling to push
`parameterChanged` to UI on DAW automation.

### Layer 1: UI

**1.1 — Cylinder component** (`ui/main.js`, `ui/styles.css`)
- SVG or Canvas cylinder with N chambers (4/6/8)
- Highlights active chamber on `chamberFired`
- Clockwise rotation animation, ~200ms per chamber advance (CSS transition)

**1.2 — Drag-drop overlay**
- Drop zone overlay with visual feedback on hover
- On successful load: shows sample name + duration
- On error: shows error message briefly

**1.3 — Controls**
- `variationAmount` knob (range input or custom SVG knob)
- `chamberCount` selector (3-option toggle: 4/6/8)
- `playbackMode` toggle (roundRobin / random)
- Generate button with disabled state (no sample) and loading state (generating)

**1.4 — State hydration**
- Handle `stateSync` to restore all controls on load
- Handle `parameterChanged` to update controls when DAW automates

### Layer 2: Dev-mode parity (ServerMain)

Implement `fileDropped` in `ServerMain.cpp`: load sample into `AudioEngine`, reply with
`sampleLoaded`/`sampleError`. Implement `generate` (for now: placeholder generation or
adapt HumanizerEngine to also work standalone). Keep `noteOn`/`noteOff` working.

## Key Design Decisions

| Decision | Choice | Alternative Rejected |
|----------|--------|---------------------|
| Pitch shift algorithm | LagrangeInterpolator two-pass | OLA (smears transients), phase vocoder (overkill) |
| HF rolloff | `juce::IIRFilter::makeLowPass` | `dsp::IIR::Filter` (needs ProcessSpec, overkill offline) |
| Thread-safe buffer swap | `atomic<shared_ptr<VariationSet>>` | Raw pointer + manual refcount (complexity) |
| File drag-drop | `FileDragAndDropTarget` on editor | HTML5 File API (blocked by WKWebView sandbox) |
| Parameter automation | APVTS | Manual `addParameter` (more boilerplate, no free ValueTree) |
| State persistence | APVTS XML + samplePath property | Separate binary blob (unnecessary) |
| UI framework | Vanilla JS/CSS | React/Vue (violates scaffold simplicity, adds build complexity) |

## Open Questions (resolved)

All NEEDS CLARIFICATION items from Technical Context resolved via research:
- ✅ Pitch shift: LagrangeInterpolator two-pass (R-001)
- ✅ HF rolloff: juce::IIRFilter (R-002)
- ✅ DAW automation: APVTS (R-003)
- ✅ File drag-drop: FileDragAndDropTarget (R-004)
- ✅ Buffer swap: atomic<shared_ptr> (R-005)
- ✅ MIDI: NEEDS_MIDI_INPUT TRUE + processBlock handling (R-006)
