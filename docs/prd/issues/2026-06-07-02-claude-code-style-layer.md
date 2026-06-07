## What to build

Add the Claude Code Style Layer and source-based display branching.

When the effective Progress Source is `codex`, the device must continue to show the Codex Core-Pulse Layer. When the effective Progress Source is `claude`, the device must show a visually distinct Claude Code Style Layer built from firmware drawing primitives. When the state is `OFFLINE` or the source is `none`, the device must show the Default Clawd Layer.

Reference PRD: `docs/prd/2026-06-07-agent-display-mode.zh-CN.md`

## Acceptance criteria

- [ ] `OFFLINE` still displays the Default Clawd Layer and keeps the offline reason in the progress message.
- [ ] `source=codex` displays the existing Codex Core-Pulse Layer.
- [ ] `source=claude` displays a distinct Claude Code Style Layer.
- [ ] Claude Code Style Layer uses `CLAUDE` as the visible label.
- [ ] Claude Code Style Layer does not reuse the Default Clawd eyes or character as its primary status visual.
- [ ] Claude Code Style Layer uses firmware drawing primitives only.
- [ ] No image, external font, official brand asset, or network dependency is introduced.
- [ ] Claude Code Style Layer reuses existing state color and phase bar semantics.
- [ ] `PLAN / CODE / TEST` have lightweight dynamic visual changes.
- [ ] `DONE` is visually stable.
- [ ] `BLOCK` uses red emphasis.
- [ ] Existing Codex Core-Pulse behavior remains unchanged for Codex states.
- [ ] Firmware source and distributed firmware mirror remain identical.
- [ ] Static contract tests cover the source-based display branch.

## Blocked by

- #2

## Non-goals

- Do not exact-clone Claude official logo, font, animation, or brand assets.
- Do not implement client-side arbitration in this issue.
- Do not change WiFi, OTA, or USB base control behavior.
- Do not auto-detect `TEST` or `BLOCK`.
- Do not delete existing watcher scripts.
