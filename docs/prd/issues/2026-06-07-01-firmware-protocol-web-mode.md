## What to build

Implement the firmware protocol and Web-controlled Display Mode foundation for multi-client work status.

The device must accept an optional Progress Source on `/progress`, expose the current source and Display Mode from `/state`, and allow the Web page to set `Auto / Codex / Claude` through `/agent-mode`. This slice establishes the end-to-end protocol path without requiring the Claude Code Style Layer or Unified Agent Watcher to be complete.

Reference PRD: `docs/prd/2026-06-07-agent-display-mode.zh-CN.md`

## Acceptance criteria

- [ ] `/progress` accepts optional `source=codex|claude|none`.
- [ ] `/progress` returns `400` for an invalid source.
- [ ] Omitting `source` updates only state and message, and does not change the current Progress Source.
- [ ] Initial Progress Source is `none`.
- [ ] USB `PROGRESS` remains source-less and does not change the current Progress Source.
- [ ] `/agent-mode?mode=auto|codex|claude` updates the runtime Display Mode.
- [ ] `/agent-mode` is case-insensitive on input and `/state` returns `AUTO|CODEX|CLAUDE`.
- [ ] Invalid Display Mode returns `400`.
- [ ] `/state` includes `progressSource` and `agentMode` while preserving existing fields.
- [ ] Web home Work Status area includes the `Auto / Codex / Claude` mode control.
- [ ] Web mode switch updates Display Mode only; it does not directly push `/progress`.
- [ ] Display Mode is not persisted across reboot and defaults to `AUTO`.
- [ ] Firmware source is maintained only in `clawd_mochi/clawd_mochi.ino`; the historical distributed firmware mirror has been removed.
- [ ] Static contract tests cover source, agent mode, and Web field behavior in the main sketch.

## Blocked by

None - can start immediately.

## Non-goals

- Do not implement the Claude Code Style Layer in this issue.
- Do not implement the Unified Agent Watcher in this issue.
- Do not delete existing `codex-watch` or `claude-watch`.
- Do not implement Windows Unified Agent Watcher.
- Do not auto-detect `TEST` or `BLOCK`.
- Do not persist Display Mode.
