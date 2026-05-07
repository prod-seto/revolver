# Research: Drum Humanizer (001)

## R-001 — Pitch Shifting with JUCE Primitives

**Decision**: Two-pass `LagrangeInterpolator` resampling (naive pitch-then-stretch)

**Rationale**: For ±5-cent variation on short drum hits rendered offline, this is the
optimal choice. Phase vocoder and OLA are overkill; OLA smears transients (unacceptable
for drums). The approach changes the effective playback ratio and then resamples back to
original length, keeping the buffer sizes consistent across all variations.

**Implementation**:
```cpp
float ratio = std::pow(2.0f, cents / 1200.0f); // e.g. 2^(5/1200) ≈ 1.0029
int shiftedLen = (int)(sourceLen / ratio);

AudioBuffer<float> shifted(channels, shiftedLen);
LagrangeInterpolator interp1;
interp1.process(ratio, src, shifted.getWritePointer(0), shiftedLen);

// Resample back to original length
float stretchRatio = (float)shiftedLen / (float)sourceLen;
AudioBuffer<float> output(channels, sourceLen);
LagrangeInterpolator interp2;
interp2.process(stretchRatio, shifted.getReadPointer(0), output.getWritePointer(0), sourceLen);
```

**Gotchas**:
- Pad source buffer by 4 extra samples to avoid read-past-end in LagrangeInterpolator
- Call `interp.reset()` between each variation to clear interpolator state
- At ±5 cents, length delta is ~0.3% of sample length — time-stretch artifact imperceptible
- LagrangeInterpolator introduces ~2 samples of delay at buffer start (irrelevant offline)

**Alternatives rejected**: Phase vocoder (too complex, overkill for drums); OLA (transient smearing).

---

## R-002 — High-Frequency Rolloff with JUCE IIRFilter

**Decision**: `juce::IIRFilter` with `juce::IIRCoefficients::makeLowPass`

**Rationale**: Simplest offline-buffer API, no `ProcessSpec` boilerplate. Provides 12 dB/oct
rolloff, which is audible and controllable. Cutoff maps: `variationAmount 0.0 → 20kHz`,
`variationAmount 1.0 → 8kHz`.

**Implementation**:
```cpp
double cutoff = 20000.0 - (amount * 12000.0); // 8kHz – 20kHz
juce::IIRFilter filter;
filter.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, cutoff));
for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    filter.reset();
    filter.processSamples(buffer.getWritePointer(ch), buffer.getNumSamples());
}
```

**Gotchas**:
- `IIRFilter` is mono — loop over channels explicitly
- Call `filter.reset()` per channel per variation (stateful)
- `dsp::IIR::Filter` is unnecessarily complex for offline use

---

## R-003 — DAW Automation: AudioProcessorValueTreeState (APVTS)

**Decision**: APVTS with three parameters + `loadedSamplePath` string stored alongside APVTS XML

**Parameters**:
| ID | Type | Range | Default |
|----|------|-------|---------|
| `variationAmount` | `AudioParameterFloat` | 0.0–1.0 | 0.5 |
| `chamberCount` | `AudioParameterChoice` | ["4","6","8"] | index 1 (="6") |
| `playbackMode` | `AudioParameterChoice` | ["roundRobin","random"] | index 0 |

**State serialization**: APVTS ValueTree + `samplePath` property → XML → `copyXmlToBinary`.
DAW automation is automatic once APVTS is wired — no additional host registration needed.

**State sync on load**: Processor sends a `stateSync` JSON message after `setStateInformation`
AND in response to a `uiReady` message (covers late editor creation during DAW project reload).

---

## R-004 — File Drag-and-Drop in JUCE WebBrowserComponent

**Decision**: `juce::FileDragAndDropTarget` on `WebPlugEditor` (OS-level interception)

**Rationale**: HTML5 drag-and-drop does not reliably receive OS file drops through
`WebBrowserComponent` — WKWebView sandboxes file drag events and `dataTransfer.files` is
typically empty. JUCE's `FileDragAndDropTarget` intercepts at the editor (native) level
before the web view, which is reliable across JUCE 7/8 on macOS.

**Implementation**: `WebPlugEditor` inherits `juce::FileDragAndDropTarget`, implements
`isInterestedInFileDrag` (filter for `.wav`/`.aif`/`.aiff`) and `filesDropped` (sends
a `fileDropped` message to `handleMessageFromUI` with the file path).

**MacOS sandbox note**: Non-sandboxed distributions (standard DAW plugin delivery) have
full read access. App Store builds would need security-scoped bookmarks — out of scope.

**Dev mode (WebSocket)**: The same `fileDropped` message type must be handled in
`ServerMain.cpp`'s `onMessage` lambda (loads sample into AudioEngine).

---

## R-005 — Off-Thread Variation Generation: Lock-Free Buffer Swap

**Decision**: `std::atomic<std::shared_ptr<VariationSet>>` — atomic pointer swap

**Rationale**: Cleanest and safest pattern for this use case. Background thread builds a
complete `VariationSet` (all variation buffers), then atomically stores it. Audio thread
loads it once per callback with no mutex acquisition on the hot path.

**Important caveat**: `atomic<shared_ptr<T>>` is NOT lock-free on Apple Silicon or x86_64
(libc++ uses an internal spinlock). However, the spinlock is held only for a pointer copy
(nanoseconds), and the background thread writes only once per generation — contention is
impossible in practice. This is acceptable for an audio plugin.

**Initialization**: Start as `nullptr`. Audio thread returns silence if the set is null
(covers the period before first generation and during re-generation).

**Data structure**:
```cpp
struct Variation {
    juce::AudioBuffer<float> buffer;
    float pitchCents;    // actual cents applied
    float gainDb;        // actual gain applied
    float hfCutoffHz;    // actual cutoff applied
};

struct VariationSet {
    std::vector<Variation> variations; // count = chamberCount
    int chamberCount;
};

std::atomic<std::shared_ptr<VariationSet>> currentSet { nullptr };
```

**Alternatives rejected**: Manual ref-counted raw pointer (unnecessary complexity);
lock-free ring buffer (overkill for a one-shot write use case).

---

## R-006 — MIDI Input and processBlock Integration

**Decision**: Enable MIDI input in CMakeLists.txt and handle MIDI in PluginProcessor

**Changes required**:
- `CMakeLists.txt`: `NEEDS_MIDI_INPUT TRUE`, `IS_SYNTH TRUE`
- `PluginProcessor.h`: `acceptsMidi()` returns `true`
- `processBlock`: iterate `MidiBuffer`, dispatch `noteOn`/`noteOff` to a new
  `VariationPlayer` class (handles chamber selection and audio rendering)
- `VariationPlayer` reads from `currentSet` atomically and mixes into the output buffer
  using the current round-robin or random index

**Round-robin state**: `std::atomic<int> nextChamberIndex` — incremented on each noteOn,
wraps at `chamberCount`. Resetting on new VariationSet load.

**Velocity scaling**: Linear gain = `velocity / 127.0f` applied to output mix.
