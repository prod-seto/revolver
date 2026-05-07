# Tasks: Revolver Drum Humanizer

**Input**: Design documents from `/specs/001-drum-humanizer/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/message-protocol.md ✅, quickstart.md ✅

**Tests**: No automated test tasks (not requested in spec). Acceptance is manual per quickstart.md.

**Organization**: Tasks grouped by user story for independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no shared dependencies)
- **[Story]**: User story this task belongs to
- File paths are relative to repo root

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Scaffold changes that must exist before any feature work begins.

- [ ] T001 Update `CMakeLists.txt`: set `NEEDS_MIDI_INPUT TRUE`, `IS_SYNTH TRUE`, add `juce::juce_audio_dsp` to plugin link libraries
- [ ] T002 Create `source/HumanizerEngine.h` with class skeleton: forward declarations for `Variation`, `VariationSet`, and `HumanizerEngine` class with empty public interface stubs
- [ ] T003 [P] Create `source/HumanizerEngine.cpp` with `#include "HumanizerEngine.h"` and empty stub implementations for all declared methods
- [ ] T004 [P] Add `source/HumanizerEngine.cpp` to `target_sources(WebPlugPlugin ...)` in `CMakeLists.txt`
- [ ] T005 [P] Replace `ui/index.html` with drum humanizer HTML skeleton: drop zone section, sample info section, cylinder placeholder, controls section, generate button — no logic yet

**Checkpoint**: Project compiles with new files; UI loads as blank skeleton in browser.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [ ] T006 Define `Variation` and `VariationSet` structs in `source/HumanizerEngine.h`: fields per data-model.md (`buffer`, `pitchCents`, `gainDb`, `hfCutoffHz`, `index` for Variation; `variations`, `chamberCount` for VariationSet)
- [ ] T007 Add `std::atomic<std::shared_ptr<VariationSet>> currentSet{nullptr}` and `std::atomic<int> nextChamberIndex{0}` to `HumanizerEngine` private members in `source/HumanizerEngine.h`
- [ ] T008 Add `juce::AudioProcessorValueTreeState apvts` member and `juce::String loadedSamplePath` to `WebPlugProcessor` in `source/PluginProcessor.h`; add `static ParameterLayout createParameterLayout()` declaration
- [ ] T009 Implement `WebPlugProcessor::createParameterLayout()` in `source/PluginProcessor.cpp`: `variationAmount` (float 0–1, default 0.5), `chamberCount` (choice ["4","6","8"], default index 1), `playbackMode` (choice ["roundRobin","random"], default index 0)
- [ ] T010 [P] Update `WebPlugProcessor` constructor initializer list in `source/PluginProcessor.cpp` to initialize `apvts(*this, nullptr, "Parameters", createParameterLayout())`
- [ ] T011 [P] Implement `getStateInformation` + `setStateInformation` in `source/PluginProcessor.cpp`: APVTS state → XML with `samplePath` property; restore APVTS then `loadedSamplePath` on load
- [ ] T012 Add `HumanizerEngine engine` member to `WebPlugProcessor` in `source/PluginProcessor.h`; add `HumanizerEngine.h` include
- [ ] T013 [P] Add `HumanizerEngine engine` local variable to `ServerMain.cpp` `main()` (alongside `AudioEngine audio` — or adapt AudioEngine to forward samples); wire notification callback stubs
- [ ] T014 [P] Scaffold stub handlers in `source/PluginProcessor.cpp` `handleMessageFromUI` for new message types: `fileDropped`, `generate`, `setParameter` — return early with no-op for now
- [ ] T015 [P] Scaffold stub handlers in `source/ServerMain.cpp` `onMessage` lambda for new message types: `fileDropped`, `generate`, `setParameter` — return early with no-op for now

**Checkpoint**: Project compiles. All three APVTS parameters visible to DAW automation. Stubs allow extending without compile breaks.

---

## Phase 3: User Story 1 — Load a Drum Sample (Priority: P1) 🎯 MVP

**Goal**: User drags a WAV/AIF file; plugin loads it, displays name and duration; errors on invalid files.

**Independent Test**: Drag `samples/kick.wav` onto the browser drop zone in dev mode; confirm `sampleLoaded` message arrives and UI shows filename and duration. Drag a `.txt` file; confirm `sampleError` and previous sample is retained.

### Implementation for User Story 1

- [ ] T016 [US1] Implement `HumanizerEngine::loadSample(const juce::File&)` in `source/HumanizerEngine.cpp`: validate extension (wav/aif/aiff), read via `juce::AudioFormatManager`, store in `sampleBuffer`, set `sampleRate`, compute duration; return struct `{bool ok, String name, double duration, String error}`
- [ ] T017 [P] [US1] Add `FileDragAndDropTarget` to `WebPlugEditor` inheritance in `source/PluginEditor.h`; declare `isInterestedInFileDrag` and `filesDropped` overrides
- [ ] T018 [US1] Implement `isInterestedInFileDrag` (accept .wav/.aif/.aiff) and `filesDropped` (build `fileDropped` JSON and call `getWebPlugProcessor().handleMessageFromUI(json)`) in `source/PluginEditor.cpp`
- [ ] T019 [US1] Implement `fileDropped` handler in `source/PluginProcessor.cpp` `handleMessageFromUI`: call `engine.loadSample(path)`, store `loadedSamplePath`, send `sampleLoaded` or `sampleError` via `onMessageToUI`
- [ ] T020 [P] [US1] Implement `fileDropped` handler in `source/ServerMain.cpp` `onMessage`: call `audio.loadSample(path)` (existing AudioEngine), send `sampleLoaded` or `sampleError` via `server.sendMessage`
- [ ] T021 [P] [US1] Implement `uiReady` handler in `source/PluginProcessor.cpp`: build full `stateSync` payload (all params + samplePath + sampleName + sampleDuration + hasVariations = false) and send via `onMessageToUI`
- [ ] T022 [P] [US1] Update `uiReady` handler in `source/ServerMain.cpp`: augment existing `audioStatus` response to send `stateSync` format instead (sampleLoaded, name, duration)
- [ ] T023 [US1] Implement drop zone UI in `ui/main.js`: register `dragover` (prevent default, add visual class) and `drop` (prevent default, send `fileDropped` via `webplug.send`) on the drop zone element
- [ ] T024 [P] [US1] Handle `sampleLoaded` in `ui/main.js`: display `payload.name` and `payload.duration` in sample info section; clear error state
- [ ] T025 [P] [US1] Handle `sampleError` in `ui/main.js`: display `payload.reason` as error indicator; retain previous sample display if any

**Checkpoint**: Drag a WAV onto the plugin (dev mode) → sample name and duration appear. Drag invalid file → error shown, sample unchanged.

---

## Phase 4: User Story 2 — Generate Humanized Variations (Priority: P2)

**Goal**: User selects chamber count (4/6/8), presses Generate; plugin produces N variation buffers off-thread; cylinder shows N active chambers.

**Independent Test**: Load a sample, press Generate with chamberCount=4 in dev mode; after `generationComplete` arrives, cylinder shows 4 active chambers. Repeat with chamberCount=8.

### Implementation for User Story 2

- [ ] T026 [US2] Implement `HumanizerEngine` background generation thread in `source/HumanizerEngine.h/.cpp`: private `juce::Thread` subclass (or `std::thread`) that calls `_generateVariations(int count, float amount)` then swaps `currentSet` atomically
- [ ] T027 [US2] Implement pitch shift DSP in `source/HumanizerEngine.cpp` `_applyPitchShift(AudioBuffer<float>& buf, float cents, double sr)`: two-pass `juce::LagrangeInterpolator` per R-001; pad source by 4 samples; call `interp.reset()` between passes
- [ ] T028 [P] [US2] Implement HF rolloff DSP in `source/HumanizerEngine.cpp` `_applyHFRolloff(AudioBuffer<float>& buf, double cutoffHz, double sr)`: `juce::IIRFilter` with `makeLowPass`, `reset()` per channel, per R-002
- [ ] T029 [P] [US2] Implement gain offset DSP in `source/HumanizerEngine.cpp` `_applyGain(AudioBuffer<float>& buf, float dB)`: `buf.applyGain(juce::Decibels::decibelsToGain(dB))`
- [ ] T030 [US2] Implement `HumanizerEngine::generateVariations(int count, float amount)` in `source/HumanizerEngine.cpp`: resample source to host sample rate if needed, then for each variation randomize pitchCents/gainDb/cutoffHz scaled by `amount`, apply DSP chain, assemble `VariationSet`, atomic swap; invoke `onGenerationStarted` before thread launch and `onGenerationComplete(count)` after swap
- [ ] T031 [US2] Add notification callbacks to `HumanizerEngine` in `source/HumanizerEngine.h`: `std::function<void()> onGenerationStarted` and `std::function<void(int)> onGenerationComplete`
- [ ] T032 [US2] Implement `generate` handler in `source/PluginProcessor.cpp`: guard (no sample → send `sampleError`), read APVTS params, call `engine.generateVariations`, set callbacks to send `generationStarted`/`generationComplete` via `onMessageToUI`
- [ ] T033 [P] [US2] Implement `generate` handler in `source/ServerMain.cpp`: same guard + callback pattern using `audio.loadSample`-loaded buffer; send `generationStarted`/`generationComplete` via `server.sendMessage` (may use AudioEngine or HumanizerEngine depending on T013 approach)
- [ ] T034 [US2] Add Generate button and chamberCount 3-way toggle (4/6/8) to `ui/index.html` if not already present from T005
- [ ] T035 [P] [US2] Implement Generate button handler in `ui/main.js`: disable button on click, send `webplug.send('generate', {})`, show progress indicator
- [ ] T036 [P] [US2] Handle `generationStarted` in `ui/main.js`: show progress spinner; handle `generationComplete` in `ui/main.js`: hide spinner, activate N chambers in cylinder, re-enable Generate button

**Checkpoint**: Generate → N chambers highlight in cylinder. variationAmount=0 → all variations sound identical to source.

---

## Phase 5: User Story 3 — MIDI-Triggered Playback (Priority: P3)

**Goal**: MIDI note-on triggers next pre-generated variation; velocity scales volume; cylinder animates active chamber.

**Independent Test**: In dev mode, press keyboard keys (keyboard.js) repeatedly; confirm `chamberFired` messages arrive cycling 0→1→2→3→0 (round-robin) and cylinder rotates. Confirm half-velocity key produces quieter output.

### Implementation for User Story 3

- [ ] T037 [US3] Define voice model in `source/HumanizerEngine.h`: `struct Voice { int variationIndex; int readPos; float gainScale; bool active; }` array of 8; `_allocateVoice()` (steal oldest if all active); `_selectNextChamber()` (round-robin or random per `playbackMode`)
- [ ] T038 [US3] Implement `HumanizerEngine::noteOn(int note, float velocity)` in `source/HumanizerEngine.cpp`: guard (no `currentSet` → return), call `_selectNextChamber()`, record fired index, allocate voice; add `std::function<void(int)> onChamberFired` callback
- [ ] T039 [P] [US3] Implement `HumanizerEngine::noteOff(int note)` in `source/HumanizerEngine.cpp`: mark matching voice inactive (one-shot: voices complete naturally; noteOff stops voice immediately per spec US3-AC5)
- [ ] T040 [US3] Update `WebPlugProcessor::acceptsMidi()` to return `true` in `source/PluginProcessor.h`; implement `processBlock` in `source/PluginProcessor.cpp`: iterate `MidiBuffer`, call `engine.noteOn/noteOff` for each event, call `engine.renderNextBlock(outputBuffer)` to mix active voices
- [ ] T041 [US3] Implement `HumanizerEngine::renderNextBlock(AudioBuffer<float>& out, int numSamples)` in `source/HumanizerEngine.cpp`: for each active voice, copy `numSamples` from `currentSet->variations[voice.variationIndex].buffer` at `voice.readPos` scaled by `voice.gainScale`; advance readPos; deactivate when done — NO allocation on this path
- [ ] T042 [US3] Send `chamberFired` from `engine.onChamberFired` callback in `source/PluginProcessor.cpp`: `juce::MessageManager::callAsync` to push `{type:"chamberFired", payload:{index:N}}` via `onMessageToUI`
- [ ] T043 [P] [US3] Wire `chamberFired` response from `noteOn` in `source/ServerMain.cpp`: when `noteOn` message arrives and variations are loaded, send `chamberFired` with the selected index
- [ ] T044 [US3] Implement cylinder SVG/canvas component in `ui/main.js`: draw N chambers arranged in a circle; highlight active chamber with a distinct fill; expose `setChamberCount(n)` and `setActiveChamber(i)` functions
- [ ] T045 [P] [US3] Add cylinder canvas element to `ui/index.html` (or confirm it exists from T005 skeleton); set appropriate dimensions in `ui/styles.css`
- [ ] T046 [US3] Implement `chamberFired` handler in `ui/main.js`: call `setActiveChamber(payload.index)` and trigger clockwise rotation animation (CSS transition ~200ms) from previous chamber to new chamber

**Checkpoint**: MIDI notes in DAW or keyboard.js in dev mode → cylinder rotates, each hit cycles chambers. Velocity difference audible.

---

## Phase 6: User Story 4 — Adjust Humanization Controls (Priority: P4)

**Goal**: UI controls for variationAmount, chamberCount, playbackMode; DAW automation reflected in UI; plugin state restores on project reload.

**Independent Test**: Toggle playbackMode roundRobin→random mid-session; verify playback immediately uses random selection without re-generating. Save and reopen DAW project; confirm all params and sample name restored without user action.

### Implementation for User Story 4

- [ ] T047 [US4] Add `AudioProcessorValueTreeState::Listener` to `WebPlugProcessor` in `source/PluginProcessor.h`; implement `parameterChanged(paramID, newValue)` to push `{type:"parameterChanged", payload:{id, value}}` via `onMessageToUI` for DAW automation updates
- [ ] T048 [P] [US4] Implement `setParameter` handler in `source/PluginProcessor.cpp` `handleMessageFromUI`: find APVTS parameter by id, set normalized value; also update `engine.setPlaybackMode()` immediately for `playbackMode` changes
- [ ] T049 [P] [US4] Implement `setParameter` handler in `source/ServerMain.cpp` `onMessage`: update local param copy; for `playbackMode` update AudioEngine/HumanizerEngine equivalent immediately
- [ ] T050 [US4] Implement `HumanizerEngine::setPlaybackMode(int mode)` in `source/HumanizerEngine.cpp`: update `std::atomic<int> playbackMode` used by `_selectNextChamber()` — immediate effect, no regeneration
- [ ] T051 [US4] Update `setStateInformation` in `source/PluginProcessor.cpp` to attempt auto-reload of `loadedSamplePath` after restoring state: if file exists call `engine.loadSample()`, then send `stateSync` (with `hasVariations: false` since buffers are not persisted)
- [ ] T052 [US4] Update `stateSync` dispatch in `source/PluginProcessor.cpp` to include current `sampleName`, `sampleDuration`, and `hasVariations` in payload (populated from engine state, not just params)
- [ ] T053 [P] [US4] Add `variationAmount` range input (0–1 step 0.01) to `ui/index.html` + wire `input` event to `webplug.send('setParameter', {id:'variationAmount', value})` in `ui/main.js`
- [ ] T054 [P] [US4] Add chamberCount 3-way toggle buttons (4/6/8) to `ui/index.html` + wire `click` event to `webplug.send('setParameter', {id:'chamberCount', value:index})` in `ui/main.js`; update cylinder chamber count on `generationComplete`
- [ ] T055 [P] [US4] Add playbackMode toggle (roundRobin/random) to `ui/index.html` + wire `click` event to `webplug.send('setParameter', {id:'playbackMode', value:index})` in `ui/main.js`
- [ ] T056 [US4] Handle `stateSync` in `ui/main.js`: hydrate all controls (variationAmount slider, chamberCount toggle, playbackMode toggle, sample name/duration, cylinder chamber count, Generate button enabled state)
- [ ] T057 [P] [US4] Handle `parameterChanged` in `ui/main.js`: update the matching control to reflect DAW automation value without triggering a `setParameter` echo

**Checkpoint**: DAW automation lane controls variationAmount knob in UI. Project save+reload → full state restored, same sample, same parameters.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Protocol parity, edge cases, and workflow gate validation.

- [ ] T058 Audit protocol parity: for each row in `specs/001-drum-humanizer/contracts/message-protocol.md` parity checklist, verify handler exists in both `source/ServerMain.cpp` and `source/PluginProcessor.cpp`; check all boxes in the table
- [ ] T059 [P] Implement edge cases in `source/HumanizerEngine.cpp`: corrupt/zero-length file (return error from `loadSample`); generate called while generation in progress (cancel/ignore with boolean guard); very short sample < 10ms (pad buffer to minimum before DSP)
- [ ] T060 [P] Implement edge case in `source/PluginProcessor.cpp`: missing sample on DAW project reload (file gone) → send `{type:"sampleError", payload:{reason:"File not found — please reload sample"}}` instead of `stateSync`
- [ ] T061 [P] Implement edge case in `source/PluginProcessor.cpp` + `source/HumanizerEngine.cpp`: MIDI note-on arrives while generation in progress → `currentSet` is null → voice not allocated, no audio output (silence), no crash
- [ ] T062 Dev loop gate: run `./scripts/dev.sh`, open `http://localhost:5173` in Chrome, perform full flow (drag WAV, generate variations, play keyboard, toggle modes, check cylinder animation); record any failures
- [ ] T063 Plugin gate: run `./scripts/install.sh`, open Logic Pro or Reaper, insert Revolver as instrument, assign MIDI track, verify VST3/AU loads, MIDI triggers chamber cycling, save+reopen project restores state; record any failures

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 completion — **BLOCKS all user stories**
- **US1 (Phase 3)**: Depends on Phase 2 — independently implementable
- **US2 (Phase 4)**: Depends on Phase 2 + US1 (needs loaded sample to generate)
- **US3 (Phase 5)**: Depends on Phase 2 + US2 (needs VariationSet to play)
- **US4 (Phase 6)**: Depends on Phase 2; `playbackMode` changes (T050) depend on US3 engine (T037)
- **Polish (Phase 7)**: Depends on all desired stories complete

### User Story Dependencies

- **US1 (P1)**: After Foundational — no story dependencies
- **US2 (P2)**: After US1 (requires sample to be loadable to test generation)
- **US3 (P3)**: After US2 (requires VariationSet to exist for playback)
- **US4 (P4)**: After Foundational; playbackMode testing requires US3; full state-sync testing requires US1+US2+US3

### Within Each Story

- C++ implementation before UI handlers (UI needs message types to be wired)
- PluginProcessor and ServerMain handlers in parallel (different files)
- `index.html` structure before `main.js` event wiring

---

## Parallel Opportunities

### Phase 2 parallel block (after T006)
```
T007  [P]  HumanizerEngine atomic members
T008  [P]  APVTS parameter declaration
T012  [P]  Add HumanizerEngine to PluginProcessor
T014  [P]  PluginProcessor message stubs
T015  [P]  ServerMain message stubs
```

### Phase 3 parallel block (after T016 loadSample)
```
T017  [P]  PluginProcessor fileDropped handler
T020  [P]  ServerMain fileDropped handler
T021  [P]  PluginProcessor uiReady/stateSync
T022  [P]  ServerMain uiReady update
T024  [P]  UI sampleLoaded handler
T025  [P]  UI sampleError handler
```

### Phase 4 parallel block (after T030 generateVariations)
```
T028  [P]  HF rolloff DSP
T029  [P]  Gain offset DSP
T033  [P]  ServerMain generate handler
T035  [P]  UI Generate button handler
T036  [P]  UI generationStarted/Complete handlers
```

### Phase 5 parallel block (after T040 processBlock)
```
T039  [P]  HumanizerEngine noteOff
T043  [P]  ServerMain chamberFired
T045  [P]  UI cylinder canvas element
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T005)
2. Complete Phase 2: Foundational (T006–T015) — **critical blocker**
3. Complete Phase 3: US1 Sample Load (T016–T025)
4. **STOP and VALIDATE**: Drag WAV onto plugin, confirm name + duration, confirm error on bad file
5. Dev loop gate (T062) scoped to sample loading only

### Incremental Delivery

1. Setup + Foundational → compiles, stubs in place
2. US1: Sample load → drag-drop works end-to-end in browser and plugin
3. US2: Variation generation → cylinder activates on generate
4. US3: MIDI playback → cylinder rotates on MIDI, audio varies
5. US4: Controls + state → DAW automation works, project saves/restores
6. Polish → protocol parity confirmed, both workflow gates pass

---

## Notes

- [P] tasks operate on different files — safe to run in parallel
- [Story] label traces each task to its user story acceptance criteria in spec.md
- No automated tests — verify each checkpoint against spec.md acceptance scenarios
- Constitution III: every message type must be wired symmetrically before PR merge (T058)
- Constitution gates: T062 (dev loop) and T063 (plugin) are the final shippability checks
- HumanizerEngine is the only new C++ file; keep all DSP there; PluginProcessor stays thin
