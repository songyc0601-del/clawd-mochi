## What to build

Update user-facing documentation so multi-client status display has one clear entry point and the old single-client watcher scripts are treated as compatibility or troubleshooting tools.

The docs should explain Display Mode, Progress Source, Unified Agent Watcher, offline behavior, Web control behavior, and the first-version scope limits.

Reference PRD: `docs/prd/2026-06-07-agent-display-mode.zh-CN.md`

## Acceptance criteria

- [ ] User manual documents the Web Display Mode control and expected screen behavior.
- [ ] User manual documents the Unified Agent Watcher as the only supported multi-client entry.
- [ ] README files point users to the unified multi-client workflow.
- [ ] Codex status sync docs describe how Codex participates through the Unified Agent Watcher.
- [ ] Claude Code status sync docs describe how Claude Code participates through the Unified Agent Watcher.
- [ ] Docs state that existing single-client watcher scripts remain available for compatibility, internal reuse, or troubleshooting.
- [ ] Docs do not recommend running Codex and Claude single-client watchers together for multi-client display.
- [ ] Docs define `Auto / Codex / Claude` behavior, including strict fixed-mode behavior.
- [ ] Docs define offline messages: `codex-offline`, `claude-offline`, `agents-offline`, and device timeout reason `codex-timeout`.
- [ ] Docs state Display Mode is runtime-only and resets to `AUTO` after reboot.
- [ ] Docs state first version excludes Windows Unified Agent Watcher.
- [ ] Docs state first version does not auto-detect `TEST` or `BLOCK`.
- [ ] Docs include a simple smoke-test path for `/agent-mode`, `/state`, and `/progress?source=...`.

## Blocked by

- #2
- #3
- #4

## Non-goals

- Do not remove compatibility watcher docs entirely.
- Do not document Windows Unified Agent Watcher as available.
- Do not document automatic `TEST` or `BLOCK` detection as implemented.
- Do not introduce new workflows outside this PRD.
