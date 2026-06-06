# Compact Web Control Page Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current control page with the approved compact, read-only work-status dashboard while preserving existing device controls and OTA behavior.

**Architecture:** Keep the existing embedded HTML approach and HTTP routes. Replace only `INDEX_HTML_LITE` and the OTA page styling, then copy the verified firmware to `dist`.

**Tech Stack:** Arduino C++, embedded HTML/CSS/JavaScript, PowerShell contract test, Arduino CLI.

---

### Task 1: Add Web Page Contract Test

**Files:**
- Create: `tools/test-web-page.ps1`
- Test: `clawd_mochi/clawd_mochi.ino`

- [ ] Write a PowerShell test that reads the firmware source and verifies the compact page contains the required controls and no browser-side manual progress action.
- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\test-web-page.ps1` and verify it fails against the old page.
- [ ] Commit the failing contract test.

### Task 2: Implement Compact Web Pages

**Files:**
- Modify: `clawd_mochi/clawd_mochi.ino`

- [ ] Replace `INDEX_HTML_LITE` with the approved compact Clawd Mochi control page.
- [ ] Make the work-status card read-only and translate progress states in JavaScript.
- [ ] Keep normal, focus, happy, backlight, OTA, and automatic state refresh.
- [ ] Simplify and visually align the OTA upload page.
- [ ] Run the contract test and verify it passes.
- [ ] Commit the firmware page update.

### Task 3: Verify and Package

**Files:**
- Modify: `dist/clawd_mochi/clawd_mochi.ino`

- [ ] Copy the verified main firmware to the `dist` firmware path.
- [ ] Compile main and `dist` with `esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200`.
- [ ] Verify main and `dist` hashes match.
- [ ] Run the contract test once more.
- [ ] Commit the synchronized distribution firmware.
