# Feature Specification: Revolver Drum Humanizer

**Feature Branch**: `001-drum-humanizer`  
**Created**: 2026-05-06  
**Status**: Draft  

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Load a Drum Sample (Priority: P1)

A producer drags a WAV or AIF drum hit file onto the plugin interface. The plugin accepts the file, reads it, and displays the sample name and duration so the user knows it loaded correctly.

**Why this priority**: Without a loaded sample there is nothing to humanize. All other features depend on this.

**Independent Test**: Load a valid WAV file via the drag-and-drop zone; the plugin displays the filename and duration. No other feature needs to be active.

**Acceptance Scenarios**:

1. **Given** the plugin is open with no sample loaded, **When** the user drags a WAV file onto the drop zone, **Then** the interface shows the sample name and its duration in seconds.
2. **Given** the plugin is open with no sample loaded, **When** the user drops an AIF file onto the drop zone, **Then** the interface shows the sample name and its duration in seconds.
3. **Given** a sample is already loaded, **When** the user drops a new file, **Then** the interface updates to show the new sample name and duration, replacing the previous one.
4. **Given** the user drops a file that is not WAV or AIF, **Then** the plugin displays an error indicator and retains any previously loaded sample.

---

### User Story 2 - Generate Humanized Variations (Priority: P2)

After loading a sample, the producer chooses how many variation slots (4, 6, or 8) to generate and presses "Generate." The plugin processes the sample and produces that many subtly different versions, ready for playback. A progress indicator informs the user that generation is underway; when complete, the cylinder UI reflects the filled chamber count.

**Why this priority**: Variation generation is the core humanization engine. Without it the plugin is just a sample player.

**Independent Test**: With a sample loaded, press Generate with chamberCount = 4; verify 4 chambers become active in the UI and each plays back distinctly when triggered.

**Acceptance Scenarios**:

1. **Given** a sample is loaded, **When** the user selects chamberCount = 4 and presses Generate, **Then** 4 variation chambers become active within a perceptible wait and the UI highlights them.
2. **Given** a sample is loaded, **When** the user selects chamberCount = 8 and presses Generate, **Then** 8 variation chambers become active and the UI reflects the larger cylinder.
3. **Given** generation is in progress, **When** the user inspects the interface, **Then** a visual indicator communicates that processing is underway.
4. **Given** no sample is loaded, **When** the user presses Generate, **Then** the button is disabled or a warning message is shown; no generation occurs.
5. **Given** variationAmount is at 0.0, **When** variations are generated, **Then** all chambers sound nearly identical to the original sample.
6. **Given** variationAmount is at 1.0, **When** variations are generated, **Then** each chamber sounds audibly distinct from the others (pitch, tone, and punch noticeably differ).

---

### User Story 3 - MIDI-Triggered Playback with Humanization (Priority: P3)

The producer routes MIDI into the plugin and plays a drum pattern. Each note-on triggers a different pre-generated variation, so the hi-hats (or kicks, snares, etc.) never sound like a machine gun — every hit feels slightly different, like a real performer.

**Why this priority**: This is the musical payoff: the plugin's entire purpose. It requires Stories 1 and 2 but delivers the end-user value.

**Independent Test**: With variations generated, send a stream of identical MIDI notes and verify that successive hits play different variation chambers. Velocity scaling can be validated with a constant-velocity MIDI sequence.

**Acceptance Scenarios**:

1. **Given** variations are generated and playbackMode = roundRobin, **When** four successive MIDI note-on events arrive, **Then** chambers cycle 0→1→2→3→0 and each note produces the corresponding variation's audio.
2. **Given** playbackMode = random, **When** repeated MIDI note-on events arrive, **Then** the chamber selection is non-sequential (statistically unlikely to stay in order across 10+ hits).
3. **Given** a MIDI note-on with full velocity (127), **When** the variation plays, **Then** the output volume is at the variation's nominal level.
4. **Given** a MIDI note-on with half velocity (64), **When** the variation plays, **Then** the output volume is perceptibly quieter (approximately half the loudness).
5. **Given** a MIDI note-off event arrives, **Then** the currently playing sample stops or completes naturally (one-shot) without new audio being triggered.
6. **Given** no variations have been generated, **When** a MIDI note-on arrives, **Then** no audio output is produced.

---

### User Story 4 - Adjust Humanization Controls (Priority: P4)

The producer tweaks variationAmount, chamberCount, and playbackMode from the plugin UI or via DAW automation. Changes to variationAmount take effect on the next Generate; changes to playbackMode and chamberCount take effect immediately in playback selection logic. The DAW can automate all parameters.

**Why this priority**: Parameters are secondary to playback but essential for expressiveness and DAW integration.

**Independent Test**: Change playbackMode toggle from roundRobin to random mid-session; verify playback selection behaviour changes without requiring a new Generate.

**Acceptance Scenarios**:

1. **Given** variations are generated, **When** the user toggles playbackMode from roundRobin to random, **Then** subsequent MIDI hits use random chamber selection immediately.
2. **Given** the user changes variationAmount and presses Generate, **Then** the newly rendered variations reflect the updated intensity level.
3. **Given** the DAW sends an automation event changing variationAmount, **Then** the UI knob updates to reflect the new value.
4. **Given** the user changes chamberCount from 4 to 8, **Then** after pressing Generate the cylinder shows 8 active chambers.
5. **Given** the plugin state is saved and reloaded by the DAW, **Then** all parameter values and the loaded sample name are restored correctly.

---

### Edge Cases

- What happens when the dropped audio file is corrupt or zero-length?
- What happens if the user presses Generate while a previous generation is still in progress?
- How does the plugin behave when chamberCount changes but Generate has not been pressed yet (stale variations)?
- What happens when a MIDI note-on arrives during variation generation (no valid buffers yet)?
- How are very short samples (< 10 ms) handled during variation generation?
- What happens when the plugin is loaded in a DAW project that references a sample file that no longer exists at its original path?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The plugin MUST accept WAV and AIF audio files via drag-and-drop onto the designated zone in the interface.
- **FR-002**: Upon receiving a valid audio file, the plugin MUST display the file's name and duration to the user.
- **FR-003**: The plugin MUST reject non-WAV/AIF files and preserve any previously loaded sample, displaying an appropriate error indication.
- **FR-004**: The user MUST be able to select a chamber count of 4, 6, or 8 before generating variations.
- **FR-005**: The plugin MUST generate the selected number of unique audio variations from the loaded sample when the user triggers generation.
- **FR-006**: Each variation MUST apply a randomized combination of subtle pitch shift, gain adjustment, transient shaping, and high-frequency rolloff; the degree of variation MUST scale with the variationAmount parameter.
- **FR-007**: The interface MUST provide visual feedback during variation generation indicating that processing is underway.
- **FR-008**: The plugin MUST respond to MIDI note-on events by playing the appropriate pre-generated variation buffer.
- **FR-009**: In round-robin playbackMode, the plugin MUST cycle through variation chambers sequentially (0 through N−1, then repeat).
- **FR-010**: In random playbackMode, the plugin MUST select a variation chamber non-deterministically on each note-on.
- **FR-011**: Output volume for each note-on MUST scale linearly with the incoming MIDI velocity.
- **FR-012**: All parameter changes (variationAmount, chamberCount, playbackMode) MUST be automatable by the host DAW.
- **FR-013**: Parameter changes from DAW automation MUST be reflected in the plugin UI in real time.
- **FR-014**: The plugin MUST send a state snapshot to the UI on load containing all current parameter values so the interface is immediately in sync.
- **FR-015**: The cylinder UI MUST visually highlight the currently active chamber and animate a rotation when a new variation fires.
- **FR-016**: The interface MUST function identically in browser dev mode (WebSocket transport) and in the DAW plugin (file:// transport) with no transport-specific UI code paths.
- **FR-017**: Variation generation MUST occur entirely off the audio thread; the audio processing path MUST never allocate memory or perform blocking operations.
- **FR-018**: The plugin MUST restore the loaded sample and all parameter values when a DAW project containing the plugin is reopened.

### Key Entities

- **Sample**: A single audio file (WAV or AIF) loaded by the user; has a name, file path, and duration.
- **Variation**: A pre-rendered audio buffer derived from the sample with applied DSP offsets; belongs to a chamber slot; has an index and associated DSP parameter values.
- **Chamber**: A slot in the cylinder; holds one variation; has an active/inactive state; is visually represented in the cylinder UI.
- **Cylinder**: The full set of N chambers (4, 6, or 8); reflects the current chamberCount; drives the rotation animation.
- **PlaybackState**: Tracks the current chamber index for round-robin mode and the overall playback status.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A producer can load a sample and generate 4 variations in under 3 seconds for a typical drum hit (< 500 ms source audio).
- **SC-002**: With 8 chambers generated, all 8 variations play back distinctly — a listener can identify them as perceptibly different on repeated audition.
- **SC-003**: MIDI note-on to audio output latency is imperceptible in a live performance context (consistent with the host DAW's audio buffer size; no additional latency introduced by the plugin).
- **SC-004**: 100% of MIDI note-on events trigger playback when variations are loaded; zero dropped notes under normal studio MIDI rates (< 100 note-on events per second).
- **SC-005**: The cylinder animation and chamber highlighting respond to MIDI triggers within one animation frame (< 16 ms) of the note-on event.
- **SC-006**: Saving and reopening a DAW project restores the plugin to the exact same state (same sample, same parameters) with no user intervention required.
- **SC-007**: The plugin UI operates without transport-specific code — the same interface works in browser dev mode and inside the DAW without modification.

## Assumptions

- The user is a music producer working in a DAW on macOS; the plugin is distributed as VST3 and AU formats.
- Only a single drum sample is loaded at a time; multi-sample layering is out of scope for this feature.
- The file path provided at sample load time is the canonical reference; sample data is not embedded in the project file (the original file must remain accessible for project reload).
- The browser dev mode (./scripts/dev.sh) targets Chromium-based browsers; Safari compatibility in dev mode is out of scope.
- variationAmount defaults to 0.5, chamberCount defaults to 6, and playbackMode defaults to roundRobin on first launch.
- Variation generation is triggered manually by the Generate button; auto-generation on sample load is a secondary behaviour (generate is triggered automatically after loadSample completes, but the user can re-generate at any time).
- The cylinder animation rotation direction is clockwise; animation duration is approximately 200 ms per chamber advance.
- MIDI note number is not used to select a specific chamber — any note-on triggers the next variation in the current playback mode. Specific note-to-chamber mapping is out of scope.
