<!--
SYNC IMPACT REPORT
==================
Version change: [template/unversioned] → 1.0.0
Bump rationale: MAJOR — initial population; all placeholders replaced with concrete project values.

Modified principles:
  - [PRINCIPLE_1_NAME] → I. Dual-Transport Unity (new)
  - [PRINCIPLE_2_NAME] → II. Web-First Development (new)
  - [PRINCIPLE_3_NAME] → III. Message Protocol Consistency (new)
  - [PRINCIPLE_4_NAME] → IV. Minimal C++ Dependencies (new)
  - [PRINCIPLE_5_NAME] → V. Scaffold Integrity (new)

Added sections:
  - Platform & Toolchain Requirements
  - Developer Workflow Gates

Removed sections:
  - None (all template stubs replaced)

Templates requiring updates:
  ✅ plan-template.md — Constitution Check gates align with dual-transport, protocol sync, and dependency constraints
  ✅ spec-template.md — No structural changes required; scope/requirements sections compatible as-is
  ✅ tasks-template.md — No structural changes required; task categories map cleanly to the five principles

Follow-up TODOs:
  - RATIFICATION_DATE set to first commit date 2026-05-06 (assumed; verify against git log if needed)
-->

# WebPlug Constitution

## Core Principles

### I. Dual-Transport Unity

The UI MUST run identically in both dev mode (WebSocket) and plugin mode (JUCE native bridge).
Transport-specific code paths in the UI are PROHIBITED — all runtime differences MUST be
encapsulated inside `bridge.js`. New features MUST be verified in both transports before
being considered complete.

Rationale: the scaffold's primary value proposition is "same code, two transports." Allowing
transport leakage into UI code invalidates the scaffold's contract for downstream plugin authors.

### II. Web-First Development

Plugin UI development MUST begin in the browser using `./scripts/dev.sh`. The browser
iteration loop (Vite hot-reload + WebSocket) is the authoritative dev environment; plugin
embedding via `WebBrowserComponent` is the delivery target.

UI code (HTML/CSS/JS) MUST NOT require a C++ rebuild to iterate. If a UI change requires
touching C++ source, that is a design smell that MUST be justified.

Rationale: hot-reload velocity is the core DX advantage this scaffold provides. Protecting
that loop ensures developers reach productive iteration quickly.

### III. Message Protocol Consistency

All communication between the UI and C++ MUST use the shared JSON message protocol:
`{ "type": "<string>", "payload": { ... } }`.

Both transport implementations — `DevServer.cpp` (dev mode) and `PluginProcessor.cpp`
(plugin mode) — MUST handle every message type. Adding a handler to one without adding
it to the other is a defect. Schemas for new message types MUST be documented before
implementation.

Rationale: divergence between the two handlers is silent and hard to detect; keeping them
in sync is a non-negotiable correctness requirement.

### IV. Minimal C++ Dependencies

The C++ layer MUST NOT introduce external C++ dependencies beyond JUCE (fetched via
CMake `FetchContent`). The WebSocket implementation MUST remain self-contained per
RFC 6455 with no third-party WebSocket library.

New C++ dependencies require explicit justification in the feature spec and MUST be
approved before implementation begins.

Rationale: external dependencies increase build complexity and onboarding friction for
plugin authors cloning the scaffold. Zero-external-dependency C++ is a deliberate design
choice.

### V. Scaffold Integrity

The scaffold MUST be buildable and runnable from a clean checkout using only the
documented prerequisites (macOS, CMake 3.22+, Xcode Command Line Tools, Node.js 18+).
`./scripts/build.sh` MUST succeed on a clean machine; `./scripts/dev.sh` MUST produce a
working dev loop; `./scripts/install.sh` MUST produce a loadable VST3/AU.

Any change that breaks the out-of-the-box experience is a P0 defect regardless of
other merits.

Rationale: the scaffold is a starting point for plugin authors. A broken first-run
experience destroys trust before the author has written a single line of their own code.

## Platform & Toolchain Requirements

- **Primary platform**: macOS. Windows support is possible but untested; PRs welcome.
- **CMake**: 3.22 or higher (required for `FetchContent` patterns used to pull JUCE).
- **Xcode Command Line Tools**: required for macOS audio plugin signing and AU validation.
- **Node.js**: 18 or higher (required by Vite and the UI toolchain).
- **JUCE**: fetched automatically via CMake `FetchContent`; no manual install required.

Changes that raise the minimum toolchain versions MUST be documented in `README.md`
and justified in the associated feature spec.

## Developer Workflow Gates

Before a feature is considered shippable, it MUST pass all three workflow gates:

1. **Dev loop gate**: `./scripts/dev.sh` runs cleanly and the ping/pong round-trip
   works in the browser UI.
2. **Plugin gate**: `./scripts/install.sh` produces a VST3/AU that loads in at least
   one DAW (Logic Pro or Reaper recommended) without errors.
3. **Protocol gate**: any new message type is handled symmetrically in both
   `DevServer.cpp` and `PluginProcessor.cpp`.

Features that pass gate 1 but not gate 2 are NOT complete — browser-only verification
is insufficient.

## Governance

This constitution supersedes all other practices for contributors to the WebPlug scaffold.
Amendments require:

1. A written rationale documenting what changes and why.
2. Verification that the amended guidance is consistent across `constitution.md`,
   `plan-template.md`, `spec-template.md`, and `tasks-template.md`.
3. A version bump following semantic versioning (MAJOR for principle removal or
   redefinition; MINOR for new principle or section; PATCH for clarifications).
4. All PRs and reviews MUST check compliance with the dual-transport and protocol
   consistency principles before merge.

Complexity introduced in violation of Principle IV (Minimal C++ Dependencies) MUST be
documented in the Complexity Tracking table of the feature's `plan.md`.

**Version**: 1.0.0 | **Ratified**: 2026-05-06 | **Last Amended**: 2026-05-06
