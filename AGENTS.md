# Codex project instructions

## Clawd Mochi status screen

For every non-trivial task in this repository, keep the connected Clawd Mochi
status screen synchronized with the current Codex phase.

Run these commands from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State PLAN -Message planning
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State CODE -Message editing
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State TEST -Message verifying
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State BLOCK -Message need-input
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State DONE -Message complete
```

- Send `PLAN` before investigation or planning.
- Send `CODE` immediately before editing files.
- Send `TEST` immediately before compilation, tests, upload, or verification.
- Send `BLOCK` only when progress genuinely requires user input.
- Send `DONE` only after the requested work and verification are complete.
- Status synchronization is best-effort. If the device is disconnected, continue
  the task without treating that as a blocker.
