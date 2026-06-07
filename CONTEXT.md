# Clawd Mochi

Clawd Mochi is a desktop companion device that shows its own default character animation and optional client work-status layers from local developer tools.

## Language

**Default Clawd Layer**:
The device's normal companion animation shown when no selected developer client is online.
_Avoid_: Offline screen, idle screen

**Work Status Layer**:
A client-specific status display shown while a developer client is online or has a recent task state.
_Avoid_: Progress page, dashboard

**Codex Core-Pulse Layer**:
The Codex work status layer built from an abstract pulsing core, state color, English status word, and phase bars.
_Avoid_: Codex face, Clawd Codex mode

**Claude Code Style Layer**:
The Claude Code work status layer inspired by Claude Code's visual identity, implemented as firmware-drawn animation rather than official image or font assets.
_Avoid_: Claude official animation, Claude logo clone

**Display Mode**:
The Web-controlled choice of which developer client is allowed to drive the work status layer: Auto, Codex, or Claude.
_Avoid_: Priority setting, source switch

**Progress Source**:
The explicit client source attached to a work status update: codex, claude, or none. Requests that omit the source do not change the current progress source.
_Avoid_: Implicit source, app name

**Auto Display Mode**:
The display mode that selects the most important current work status across developer clients, using recency only to break ties at the same status level.
_Avoid_: Latest-client mode, Codex-first mode

**Unified Agent Watcher**:
The only supported multi-client entry point; it observes multiple developer clients, resolves the display mode, and sends one final status to the device.
_Avoid_: Dual watcher, priority script
