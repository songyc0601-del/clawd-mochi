## What to build

Implement the Unified Agent Watcher as the only supported multi-client entry point.

The watcher must observe Codex and Claude Code session activity, read the device Display Mode by default, choose exactly one final work status, and push that status to the device with an explicit Progress Source. This prevents competing watcher updates when Codex and Claude Code run at the same time.

Reference PRD: `docs/prd/2026-06-07-agent-display-mode.zh-CN.md`

## Acceptance criteria

- [ ] A single Linux/macOS/WSL watcher command can observe both Codex and Claude Code.
- [ ] The watcher supports default device-controlled mode and debug overrides for `auto`, `codex`, and `claude`.
- [ ] The watcher reads `/state.agentMode` by default.
- [ ] If `/state` fails, the watcher uses the last successfully read mode.
- [ ] If `/state` has never succeeded, the watcher uses `AUTO`.
- [ ] In `AUTO`, selection uses `BLOCK > TEST > CODE > PLAN > DONE > IDLE > OFFLINE`.
- [ ] Same-priority states are resolved by most recent activity.
- [ ] Fully tied states resolve deterministically as `Codex > Claude Code`.
- [ ] In fixed Codex mode, Claude Code cannot take over the screen.
- [ ] In fixed Claude mode, Codex cannot take over the screen.
- [ ] If the fixed-mode client is offline, the watcher sends an offline status that returns the device to the Default Clawd Layer.
- [ ] Both clients offline emits `source=none`, `OFFLINE`, `agents-offline`.
- [ ] Codex-only offline reason uses `codex-offline`.
- [ ] Claude-only offline reason uses `claude-offline`.
- [ ] Long-running watcher re-sends the unchanged final status every 60 seconds by default.
- [ ] Long-running watcher exit sends `OFFLINE agents-offline`.
- [ ] `--once` exit does not send the exit offline status.
- [ ] Background controls are available for start, stop, and status.
- [ ] Public behavior tests cover Auto, fixed modes, offline handling, heartbeat, and `--once`.
- [ ] Existing single-client watcher tests remain passing.

## Blocked by

- #2

## Non-goals

- Do not implement Windows Unified Agent Watcher in this issue.
- Do not remove existing `codex-watch` or `claude-watch`.
- Do not auto-detect real `TEST` or `BLOCK`; only preserve priority and forwarding semantics.
- Do not implement additional clients.
- Do not move arbitration into ESP32 firmware.
