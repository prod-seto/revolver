# Data Model: Drum Humanizer (001)

## Entities

### Sample

Represents the single loaded audio file. Only one Sample exists at a time.

| Field | Type | Description |
|-------|------|-------------|
| `filePath` | `juce::String` | Absolute path on disk; canonical reference for project reload |
| `fileName` | `juce::String` | Display name (file name without directory) |
| `durationSeconds` | `double` | Total duration in seconds |
| `sampleRate` | `double` | Native sample rate of the file |
| `numChannels` | `int` | Channel count of the source file |
| `buffer` | `juce::AudioBuffer<float>` | Raw PCM data at source sample rate |

**Validation rules**:
- Extension MUST be `.wav`, `.aif`, or `.aiff` (case-insensitive)
- File MUST be readable and non-zero length
- On project reload, if `filePath` does not exist, the plugin enters "missing sample" state
  (parameters restored, playback disabled, UI shows warning)

**State transitions**:
```
unloaded → loading → loaded → (replaced by new Sample on next drop)
                  ↓ (file invalid)
              loadFailed (previous sample retained if any)
```

---

### Variation

A pre-rendered audio buffer derived from the Sample with applied DSP offsets. Variations
are immutable once generated; re-generation replaces the entire VariationSet.

| Field | Type | Description |
|-------|------|-------------|
| `index` | `int` | 0-based position in the cylinder (0 to chamberCount-1) |
| `buffer` | `juce::AudioBuffer<float>` | Pre-rendered PCM at host sample rate |
| `pitchCents` | `float` | Pitch offset applied (–5 to +5 cents, random) |
| `gainDb` | `float` | Gain offset applied (–1 to +1 dB, random) |
| `hfCutoffHz` | `double` | Low-pass cutoff applied (8kHz–20kHz, random) |

**Derivation**: DSP parameter values are randomized per-variation at generation time, then
scaled by `variationAmount`. At `variationAmount = 0.0`, all values collapse to neutral
(0 cents, 0 dB, 20kHz). At `variationAmount = 1.0`, full range is used.

---

### VariationSet

The complete collection of Variations for a single generation pass. Held as a
`std::shared_ptr` and swapped atomically when generation completes.

| Field | Type | Description |
|-------|------|-------------|
| `variations` | `std::vector<Variation>` | Ordered list of N variations |
| `chamberCount` | `int` | N — the count at generation time (4, 6, or 8) |

**Invariant**: `variations.size() == chamberCount` after successful generation.

---

### Chamber

A logical slot in the cylinder. Derived from the VariationSet; not stored independently.

| Field | Type | Description |
|-------|------|-------------|
| `index` | `int` | Position in cylinder (0 to chamberCount-1) |
| `active` | `bool` | True when a variation occupies this chamber |
| `isCurrent` | `bool` | True for the chamber that fired last |

**Representation**: Computed from `VariationSet` and `PlaybackState` by the UI on each
`stateSync` or `chamberFired` message.

---

### PlaybackState

Tracks the runtime state of variation playback. Maintained in the audio processor.

| Field | Type | Description |
|-------|------|-------------|
| `mode` | `enum { roundRobin, random }` | Selection algorithm for next variation |
| `nextIndex` | `std::atomic<int>` | Next chamber to fire in round-robin mode |
| `activeVoices` | `fixed array` | Active voice playback positions (up to 8) |

**Mode transitions**: `playbackMode` parameter change takes effect immediately — no
regeneration required.

**nextIndex wrapping**: On each note-on, `nextIndex = (nextIndex + 1) % chamberCount`.
Reset to 0 when a new VariationSet is installed.

---

### Parameters (APVTS)

DAW-automatable parameters managed by `AudioProcessorValueTreeState`.

| ID | Label | Type | Range | Default | Effect |
|----|-------|------|-------|---------|--------|
| `variationAmount` | Variation Amount | float | 0.0–1.0 | 0.5 | Scales DSP offset range at generation time |
| `chamberCount` | Chamber Count | choice | 4/6/8 | 6 | Number of variations generated; affects cylinder size |
| `playbackMode` | Playback Mode | choice | roundRobin/random | roundRobin | Variation selection algorithm (immediate effect) |

---

## State Persistence

**What is saved** (in `getStateInformation`):
1. APVTS ValueTree (all three parameters)
2. `samplePath` property on the ValueTree root

**What is NOT saved** (ephemeral):
- Variation audio buffers (re-generated on next explicit Generate)
- PlaybackState.nextIndex (resets to 0 on reload)

**Project reload sequence**:
1. DAW calls `setStateInformation` → parameters restored, `loadedSamplePath` set
2. Processor attempts to load sample from `loadedSamplePath`
3. If file exists: sample loaded, `stateSync` message sent to UI
4. If file missing: `stateSyncMissingSample` message sent to UI; parameters still restored

---

## Message Protocol

All messages use the shared format: `{ "type": "<string>", "payload": { ... } }`.
Each type below must be handled symmetrically in both `ServerMain.cpp` and `PluginProcessor.cpp`.

| Direction | Type | Payload | Description |
|-----------|------|---------|-------------|
| UI → C++ | `fileDropped` | `{ path: string }` | OS drag-drop of audio file onto editor |
| UI → C++ | `generate` | `{}` | Trigger variation generation with current params |
| UI → C++ | `setParameter` | `{ id: string, value: number }` | Parameter change from UI knob/toggle |
| UI → C++ | `uiReady` | `{}` | UI has loaded; C++ should send stateSync |
| UI → C++ | `noteOn` | `{ note: int, velocity: float }` | Dev-mode MIDI trigger |
| UI → C++ | `noteOff` | `{ note: int }` | Dev-mode note release |
| C++ → UI | `stateSync` | `{ variationAmount, chamberCount, playbackMode, samplePath, sampleName, sampleDuration, hasVariations, chamberCount }` | Full state snapshot |
| C++ → UI | `sampleLoaded` | `{ name: string, duration: number }` | Sample accepted |
| C++ → UI | `sampleError` | `{ reason: string }` | Sample rejected or missing |
| C++ → UI | `generationStarted` | `{}` | Variation generation begun |
| C++ → UI | `generationComplete` | `{ chamberCount: int }` | Variations ready |
| C++ → UI | `chamberFired` | `{ index: int }` | Note-on triggered chamber index |
| C++ → UI | `parameterChanged` | `{ id: string, value: number }` | DAW automation update |
