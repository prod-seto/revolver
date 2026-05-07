# Message Protocol Contract: Drum Humanizer (001)

All messages use the envelope: `{ "type": "<string>", "payload": { ... } }`

**Constitution III requirement**: Every message type below MUST be handled in BOTH
`source/DevServer.cpp` (via `ServerMain.cpp` `onMessage` lambda) AND
`source/PluginProcessor.cpp` (`handleMessageFromUI`). Adding a handler to one without
the other is a defect.

---

## UI → C++ Messages

### `uiReady`
Sent automatically by `bridge.js` on `DOMContentLoaded`. C++ responds with `stateSync`.

```json
{ "type": "uiReady", "payload": {} }
```

---

### `fileDropped`
Sent by `PluginEditor.cpp` `filesDropped` callback (OS drag-drop) or by the UI if a
file picker is used in dev mode.

```json
{
  "type": "fileDropped",
  "payload": { "path": "/absolute/path/to/kick.wav" }
}
```

**Response**: `sampleLoaded` on success; `sampleError` on failure.

---

### `generate`
Triggers off-thread variation generation using current parameter values.

```json
{ "type": "generate", "payload": {} }
```

**Precondition**: Sample must be loaded. If not, C++ returns `sampleError`.
**Response**: `generationStarted` immediately, then `generationComplete` when done.

---

### `setParameter`
Parameter value changed from the UI (before DAW automation is wired). Value is the
normalised float for `variationAmount` (0.0–1.0); for `chamberCount` and `playbackMode`
it is the choice index (0-based integer as float).

```json
{
  "type": "setParameter",
  "payload": { "id": "variationAmount", "value": 0.75 }
}
```

Valid `id` values: `"variationAmount"`, `"chamberCount"`, `"playbackMode"`

**Response**: None. DAW automation will send `parameterChanged` back in the other direction.

---

### `noteOn` *(dev mode only)*
Simulates a MIDI note-on for keyboard/button testing in the browser.

```json
{
  "type": "noteOn",
  "payload": { "note": 60, "velocity": 0.85 }
}
```

---

### `noteOff` *(dev mode only)*

```json
{
  "type": "noteOff",
  "payload": { "note": 60 }
}
```

---

## C++ → UI Messages

### `stateSync`
Full state snapshot. Sent on `uiReady` and after `setStateInformation`.

```json
{
  "type": "stateSync",
  "payload": {
    "variationAmount": 0.5,
    "chamberCount": 1,
    "playbackMode": 0,
    "samplePath": "/Users/producer/Samples/kick.wav",
    "sampleName": "kick.wav",
    "sampleDuration": 0.43,
    "hasVariations": false
  }
}
```

`chamberCount` payload is the choice index (0=4, 1=6, 2=8). `playbackMode` is 0 or 1.
`samplePath` is empty string if no sample loaded.

---

### `sampleLoaded`

```json
{
  "type": "sampleLoaded",
  "payload": { "name": "kick.wav", "duration": 0.43 }
}
```

---

### `sampleError`

```json
{
  "type": "sampleError",
  "payload": { "reason": "File not found" }
}
```

Possible reasons: `"File not found"`, `"Unsupported format"`, `"File is empty"`,
`"Corrupt or unreadable"`.

---

### `generationStarted`

```json
{ "type": "generationStarted", "payload": {} }
```

---

### `generationComplete`

```json
{
  "type": "generationComplete",
  "payload": { "chamberCount": 6 }
}
```

`chamberCount` is the actual number of variations generated (integer 4, 6, or 8).

---

### `chamberFired`
Sent on each MIDI note-on (or dev-mode noteOn) once variations are loaded.

```json
{
  "type": "chamberFired",
  "payload": { "index": 2 }
}
```

`index` is 0-based. The UI uses this to drive the cylinder rotation animation and
highlight the active chamber.

---

### `parameterChanged`
Sent when a DAW automation event modifies a parameter value.

```json
{
  "type": "parameterChanged",
  "payload": { "id": "variationAmount", "value": 0.8 }
}
```

The UI updates the corresponding control without triggering a `setParameter` echo.

---

## Protocol Parity Checklist

Before merging, verify each type is handled in both transport implementations:

| Message Type | DevServer (ServerMain.cpp) | PluginProcessor.cpp |
|---|---|---|
| `uiReady` | ✗ TODO | ✗ TODO |
| `fileDropped` | ✗ TODO | ✗ TODO |
| `generate` | ✗ TODO | ✗ TODO |
| `setParameter` | ✗ TODO | ✗ TODO |
| `noteOn` | ✓ (exists) | ✗ TODO |
| `noteOff` | ✓ (exists) | ✗ TODO |
| `ping` | ✓ (exists) | ✓ (exists) |
