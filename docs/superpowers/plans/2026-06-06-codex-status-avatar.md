# Codex Status Avatar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a Codex core-pulse status screen that appears only while Codex is online, and returns to the default Clawd animation when Codex is offline.

**Architecture:** Keep the existing firmware protocol compatible and add `OFFLINE` as a safe status alias for “return to default view.” The firmware owns display semantics: `OFFLINE` draws the default Clawd animation, while `IDLE / PLAN / CODE / TEST / DONE / BLOCK` draw the Codex core-pulse layer. The client watcher owns lifecycle hints by pushing `IDLE codex-ready` on startup and `OFFLINE codex-offline` when no Codex session is available or the watcher exits.

**Tech Stack:** Arduino C++ for ESP32-C3, Adafruit ST7789 drawing primitives, Bash/PowerShell watcher scripts, repository static shell tests. Do not run Arduino compilation while the ESP32 core is still missing or downloading.

---

## Files

- Modify: `clawd_mochi/clawd_mochi.ino`
  - Add `OFFLINE` status handling.
  - Replace the old progress dashboard with the Codex core-pulse screen.
  - Keep default Clawd animation as the offline/default display.
  - Keep `/progress`, `/state`, and USB `PROGRESS` compatibility.
- Modify: `dist/clawd_mochi/clawd_mochi.ino`
  - Mirror the firmware changes exactly.
- Modify: `tools/codex-stage.sh`
  - Accept `OFFLINE` in help text.
- Modify: `tools/codex-stage.ps1`
  - Accept `OFFLINE` in `ValidateSet`.
- Modify: `tools/codex-watch.sh`
  - Push `IDLE codex-ready` when a Codex session directory exists but no active task is detected.
  - Push `OFFLINE codex-offline` when no session directory/file exists or when the watcher exits.
- Modify: `tools/codex-watch.ps1`
  - Mirror the watcher lifecycle behavior for Windows.
- Create: `tools/test-codex-status-avatar.sh`
  - Static contract test for the firmware and scripts.
- Modify: `README.zh-CN.md`
  - Document `OFFLINE` and the new display behavior.
- Modify: `docs/codex-client-status-sync.zh-CN.md`
  - Document watcher lifecycle mapping.
- Modify: `docs/usb-serial-codex.zh-CN.md`
  - Document `PROGRESS OFFLINE`.

---

### Task 1: Add Static Contract Test

**Files:**
- Create: `tools/test-codex-status-avatar.sh`

- [ ] **Step 1: Create a failing static test**

Create `tools/test-codex-status-avatar.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
FW="$ROOT/clawd_mochi/clawd_mochi.ino"
DIST="$ROOT/dist/clawd_mochi/clawd_mochi.ino"
STAGE_SH="$ROOT/tools/codex-stage.sh"
WATCH_SH="$ROOT/tools/codex-watch.sh"

for file in "$FW" "$DIST" "$STAGE_SH" "$WATCH_SH"; do
  if [ ! -f "$file" ]; then
    echo "Missing required file: $file" >&2
    exit 1
  fi
done

required_fw=(
  'const String PROGRESS_OFFLINE = "OFFLINE";'
  'bool isCodexLayerState(const String& state)'
  'void drawCodexCore(uint16_t col, uint8_t pulse)'
  'void drawProgressBars(uint8_t stage, uint16_t col)'
  'String progressDefaultMessage(const String& state)'
  'if (state == PROGRESS_OFFLINE)'
  'drawNormalEyes()'
  'progressPulsePhase'
  'Codex core-pulse status layer'
)

for text in "${required_fw[@]}"; do
  if ! grep -Fq "$text" "$FW"; then
    echo "Firmware missing Codex status avatar contract: $text" >&2
    exit 1
  fi
  if ! grep -Fq "$text" "$DIST"; then
    echo "Dist firmware missing Codex status avatar contract: $text" >&2
    exit 1
  fi
done

if ! grep -Fq 'States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE' "$STAGE_SH"; then
  echo "codex-stage.sh help must list OFFLINE" >&2
  exit 1
fi

required_watch=(
  'push_state "IDLE" "codex-ready"'
  'push_state "OFFLINE" "codex-offline"'
  'trap cleanup EXIT INT TERM'
)

for text in "${required_watch[@]}"; do
  if ! grep -Fq "$text" "$WATCH_SH"; then
    echo "codex-watch.sh missing lifecycle contract: $text" >&2
    exit 1
  fi
done

if ! cmp -s "$FW" "$DIST"; then
  echo "clawd_mochi and dist firmware must stay identical" >&2
  exit 1
fi

echo "Codex status avatar contract passed"
```

- [ ] **Step 2: Make the test executable**

Run:

```bash
chmod +x tools/test-codex-status-avatar.sh
```

- [ ] **Step 3: Run test to verify it fails before implementation**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected: FAIL with a message like:

```text
Firmware missing Codex status avatar contract: const String PROGRESS_OFFLINE = "OFFLINE";
```

- [ ] **Step 4: Commit the failing contract test**

Run:

```bash
git add tools/test-codex-status-avatar.sh
git commit -m "添加 Codex 状态形象屏契约测试"
```

---

### Task 2: Implement Firmware State Semantics and Core-Pulse Drawing

**Files:**
- Modify: `clawd_mochi/clawd_mochi.ino`
- Modify: `dist/clawd_mochi/clawd_mochi.ino`

- [ ] **Step 1: Add status constants and helpers near progress globals**

Find the existing progress globals:

```cpp
String   progressState = "IDLE";
String   progressMsg   = "";
```

Replace them with:

```cpp
const String PROGRESS_OFFLINE = "OFFLINE";
const String PROGRESS_IDLE    = "IDLE";
const String PROGRESS_PLAN    = "PLAN";
const String PROGRESS_CODE    = "CODE";
const String PROGRESS_TEST    = "TEST";
const String PROGRESS_DONE    = "DONE";
const String PROGRESS_BLOCK   = "BLOCK";

String   progressState = PROGRESS_OFFLINE;
String   progressMsg   = "";
uint8_t  progressPulsePhase = 0;
```

- [ ] **Step 2: Replace progress state validation**

Replace `isProgressState()` with:

```cpp
bool isCodexLayerState(const String& state) {
  return state == PROGRESS_IDLE || state == PROGRESS_PLAN || state == PROGRESS_CODE ||
         state == PROGRESS_TEST || state == PROGRESS_DONE || state == PROGRESS_BLOCK;
}

bool isProgressState(const String& state) {
  return state == PROGRESS_OFFLINE || isCodexLayerState(state);
}
```

- [ ] **Step 3: Update progress color and stage helpers**

Place these helpers near `progressColor()`:

```cpp
uint16_t progressColor(const String& state) {
  if (state == PROGRESS_OFFLINE) return C_ORANGE;
  if (state == PROGRESS_IDLE)    return tft.color565(110, 116, 124);
  if (state == PROGRESS_PLAN)    return tft.color565(70, 130, 220);
  if (state == PROGRESS_CODE)    return C_ORANGE;
  if (state == PROGRESS_TEST)    return tft.color565(50, 208, 176);
  if (state == PROGRESS_DONE)    return tft.color565(40, 200, 100);
  if (state == PROGRESS_BLOCK)   return tft.color565(230, 60, 40);
  return C_MUTED;
}

uint8_t progressStage(const String& state) {
  if (state == PROGRESS_PLAN)  return 1;
  if (state == PROGRESS_CODE)  return 2;
  if (state == PROGRESS_TEST)  return 3;
  if (state == PROGRESS_DONE)  return 4;
  if (state == PROGRESS_BLOCK) return 4;
  return 0;
}

String progressDefaultMessage(const String& state) {
  if (state == PROGRESS_IDLE)  return "codex-ready";
  if (state == PROGRESS_PLAN)  return "planning";
  if (state == PROGRESS_CODE)  return "active";
  if (state == PROGRESS_TEST)  return "verifying";
  if (state == PROGRESS_DONE)  return "turn-complete";
  if (state == PROGRESS_BLOCK) return "need-input";
  return "";
}
```

- [ ] **Step 4: Add core-pulse drawing helpers**

Place these before `drawProgressView()`:

```cpp
void drawProgressBars(uint8_t stage, uint16_t col) {
  const uint16_t off = tft.color565(48, 50, 56);
  const int16_t x0 = 30;
  const int16_t y = 148;
  const int16_t w = 39;
  const int16_t h = 8;
  const int16_t gap = 6;
  for (uint8_t i = 0; i < 4; i++) {
    const int16_t x = x0 + i * (w + gap);
    tft.fillRoundRect(x, y, w, h, 3, i < stage ? col : off);
  }
}

void drawCodexCore(uint16_t col, uint8_t pulse) {
  const int16_t cx = 120;
  const int16_t cy = 84;
  const uint8_t outer = 34 + pulse * 3;
  const uint8_t mid = 22 + pulse;
  const uint16_t dim = tft.color565(36, 38, 44);

  tft.fillRect(58, 34, 124, 96, C_DARKBG);
  tft.drawCircle(cx, cy, outer, col);
  tft.drawCircle(cx, cy, outer + 8, dim);
  tft.drawCircle(cx, cy, mid, col);
  tft.fillCircle(cx, cy, 10 + pulse, col);
}

void drawCodexScanLine(uint16_t col, uint8_t pulse) {
  const int16_t y = 66 + pulse * 10;
  tft.drawFastHLine(78, y, 84, col);
}
```

- [ ] **Step 5: Replace `drawProgressView()`**

Replace the current `drawProgressView()` body with:

```cpp
void drawProgressView() {
  termMode = false;

  if (progressState == PROGRESS_OFFLINE) {
    showNormal();
    return;
  }

  currentView = VIEW_PROGRESS;
  const uint16_t col = progressColor(progressState);
  const uint8_t stage = progressStage(progressState);
  const String msg = progressMsg.length() > 0 ? progressMsg : progressDefaultMessage(progressState);
  const uint8_t pulse = (progressState == PROGRESS_DONE) ? 1 : progressPulsePhase;

  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 7, col);
  tft.fillRect(0, DISP_H - 7, DISP_W, 7, col);

  tft.setTextColor(C_MUTED); tft.setTextSize(1);
  tft.setCursor(14, 18); tft.print("CODEX");

  tft.fillCircle(218, 22, 5, col);
  drawCodexCore(col, pulse);
  if (progressState == PROGRESS_TEST) drawCodexScanLine(col, pulse);
  drawProgressBars(stage, col);

  tft.setTextColor(col); tft.setTextSize(4);
  int16_t x = (DISP_W - progressState.length() * 24) / 2;
  if (x < 0) x = 0;
  tft.setCursor(x, 172); tft.print(progressState);

  if (msg.length() > 0) {
    tft.setTextColor(C_WHITE); tft.setTextSize(1);
    int16_t msgX = (DISP_W - msg.length() * 6) / 2;
    if (msgX < 8) msgX = 8;
    tft.setCursor(msgX, 214); tft.print(msg);
  }
}
```

- [ ] **Step 6: Update `setProgress()` and startup default**

In `setProgress()`, keep validation and add the offline branch:

```cpp
bool setProgress(String state, String msg) {
  state.trim();
  state.toUpperCase();
  if (!isProgressState(state)) return false;
  progressState = state;
  progressMsg = cleanAscii(msg, 24);
  progressBlinkOn = true;
  progressPulsePhase = 0;
  lastProgressBlinkMs = millis();
  drawProgressView();
  return true;
}
```

In `setup()`, replace:

```cpp
progressState = "IDLE";
```

with:

```cpp
progressState = PROGRESS_OFFLINE;
```

- [ ] **Step 7: Replace `progressTick()`**

Replace the current `progressTick()` with:

```cpp
void progressTick() {
  if (currentView != VIEW_PROGRESS || !isCodexLayerState(progressState)) return;
  const uint32_t now = millis();
  if (now - lastProgressBlinkMs < 550) return;
  lastProgressBlinkMs = now;

  if (progressState == PROGRESS_DONE) return;

  progressBlinkOn = !progressBlinkOn;
  progressPulsePhase = (progressPulsePhase + 1) % 3;

  const uint16_t col = progressColor(progressState);
  const uint8_t pulse = progressState == PROGRESS_BLOCK && !progressBlinkOn ? 0 : progressPulsePhase;
  drawCodexCore(col, pulse);
  if (progressState == PROGRESS_TEST) drawCodexScanLine(col, pulse);
  tft.fillCircle(218, 22, 5, progressBlinkOn ? col : tft.color565(48, 50, 56));
}
```

- [ ] **Step 8: Mirror firmware file**

Run:

```bash
cp clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
```

- [ ] **Step 9: Run static test**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected:

```text
Codex status avatar contract passed
```

- [ ] **Step 10: Commit firmware display changes**

Run:

```bash
git add clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
git commit -m "实现 Codex 核心脉冲状态屏"
```

---

### Task 3: Update Client Status Scripts for OFFLINE and Codex-Ready

**Files:**
- Modify: `tools/codex-stage.sh`
- Modify: `tools/codex-stage.ps1`
- Modify: `tools/codex-watch.sh`
- Modify: `tools/codex-watch.ps1`

- [ ] **Step 1: Update shell stage help**

In `tools/codex-stage.sh`, replace:

```text
States: IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE
```

with:

```text
States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE
```

- [ ] **Step 2: Update PowerShell stage validation**

In `tools/codex-stage.ps1`, replace the `ValidateSet` with:

```powershell
[ValidateSet("OFFLINE", "IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK", "STATE")]
```

- [ ] **Step 3: Add watcher cleanup in shell watcher**

After variable initialization in `tools/codex-watch.sh`, add:

```bash
cleanup() {
  push_state "OFFLINE" "codex-offline"
}

trap cleanup EXIT INT TERM
```

- [ ] **Step 4: Change no-session handling in shell watcher**

In `poll_once()`, replace the no-session branch with:

```bash
  if [ -z "$latest" ]; then
    if [ "$current_state" != "OFFLINE" ]; then
      push_state "OFFLINE" "codex-offline"
      current_state="OFFLINE"
    fi
    return 0
  fi
```

- [ ] **Step 5: Change stale-session handling in shell watcher**

In the new-session branch, replace the stale case:

```bash
      push_state "IDLE" "idle"
      current_state="IDLE"
```

with:

```bash
      push_state "OFFLINE" "codex-offline"
      current_state="OFFLINE"
```

- [ ] **Step 6: Change long inactivity handling in shell watcher**

Replace the long inactivity push:

```bash
    push_state "IDLE" "idle"
    current_state="IDLE"
```

with:

```bash
    push_state "IDLE" "codex-ready"
    current_state="IDLE"
```

- [ ] **Step 7: Update Windows watcher validation and cleanup**

In `tools/codex-watch.ps1`, set the state validator to:

```powershell
[ValidateSet("OFFLINE", "IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK")]
```

Register cleanup after function definitions:

```powershell
Register-EngineEvent PowerShell.Exiting -Action {
  try {
    & $StageScript -DeviceUrl $DeviceUrl -State OFFLINE -Message "codex-offline" | Out-Null
  } catch {}
} | Out-Null
```

Change no-session and stale-session pushes from `IDLE no-session` or `IDLE idle` to:

```powershell
Push-State -State OFFLINE -Message "codex-offline"
$script:currentState = "OFFLINE"
```

Change long inactivity to:

```powershell
Push-State -State IDLE -Message "codex-ready"
$script:currentState = "IDLE"
```

- [ ] **Step 8: Run script syntax checks**

Run:

```bash
bash -n tools/codex-stage.sh
bash -n tools/codex-watch.sh
./tools/codex-watch.sh --once --verbose --stage-script /bin/true
./tools/test-codex-status-avatar.sh
```

Expected:

```text
[codex-watch] push ...
Codex status avatar contract passed
```

PowerShell scripts are not run in WSL unless `pwsh` or Windows PowerShell is available.

- [ ] **Step 9: Commit script changes**

Run:

```bash
git add tools/codex-stage.sh tools/codex-stage.ps1 tools/codex-watch.sh tools/codex-watch.ps1
git commit -m "支持 Codex 离线回退状态"
```

---

### Task 4: Update Documentation

**Files:**
- Modify: `README.zh-CN.md`
- Modify: `docs/codex-client-status-sync.zh-CN.md`
- Modify: `docs/usb-serial-codex.zh-CN.md`

- [ ] **Step 1: Update Chinese README status section**

In `README.zh-CN.md`, update the Codex status description to include:

```markdown
- Codex 未打开、watcher 停止或离线时，设备显示默认 Clawd 动画形象。
- Codex 打开但暂无任务时，显示 `IDLE` Codex 核心脉冲待机屏。
- `PLAN / CODE / TEST / DONE / BLOCK` 显示 Codex 核心脉冲状态屏。
- `OFFLINE` 可用于脚本显式请求回到默认 Clawd 动画形象。
```

- [ ] **Step 2: Update client sync document**

In `docs/codex-client-status-sync.zh-CN.md`, update watcher behavior to:

```markdown
- watcher 启动并发现 Codex session：推送 `IDLE codex-ready`，表示 Codex 在线待机。
- session 文件近期变化：推送 `PLAN codex-session`，随后推送 `CODE active`。
- 默认 20 秒无变化：推送 `DONE turn-complete`。
- 默认 300 秒无变化：推送 `IDLE codex-ready`。
- 未发现 session、watcher 退出或 Codex 离线：推送 `OFFLINE codex-offline`，设备回到默认 Clawd 动画形象。
```

- [ ] **Step 3: Update USB serial document**

In `docs/usb-serial-codex.zh-CN.md`, add `OFFLINE` to the command examples:

```text
PROGRESS OFFLINE codex-offline
PROGRESS IDLE codex-ready
PROGRESS PLAN planning
PROGRESS CODE active
PROGRESS TEST verifying
PROGRESS DONE turn-complete
PROGRESS BLOCK need-input
```

Add the behavior note:

```markdown
`OFFLINE` 不表示 Codex 的工作阶段；它表示 Codex 客户端离线或 watcher 停止，设备应回到默认 Clawd 动画形象。
```

- [ ] **Step 4: Run documentation/static checks**

Run:

```bash
./tools/test-codex-status-avatar.sh
rg -n "OFFLINE|codex-ready|codex-offline" README.zh-CN.md docs/codex-client-status-sync.zh-CN.md docs/usb-serial-codex.zh-CN.md
```

Expected:

```text
Codex status avatar contract passed
```

and `rg` prints matches in all three docs.

- [ ] **Step 5: Commit documentation changes**

Run:

```bash
git add README.zh-CN.md docs/codex-client-status-sync.zh-CN.md docs/usb-serial-codex.zh-CN.md
git commit -m "更新 Codex 状态屏离线说明"
```

---

### Task 5: Final Verification Without Arduino Compile

**Files:**
- No source files created.

- [ ] **Step 1: Run shell syntax checks**

Run:

```bash
bash -n tools/codex-stage.sh
bash -n tools/codex-watch.sh
```

Expected: no output and exit code 0.

- [ ] **Step 2: Run watcher dry-run**

Run:

```bash
./tools/codex-watch.sh --once --verbose --stage-script /bin/true
```

Expected: a verbose `push ...` line. The exact state depends on local Codex session files.

- [ ] **Step 3: Run contract test**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected:

```text
Codex status avatar contract passed
```

- [ ] **Step 4: Confirm firmware mirror**

Run:

```bash
cmp -s clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
echo $?
```

Expected:

```text
0
```

- [ ] **Step 5: Confirm no Arduino compile was run**

Do not run:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 clawd_mochi
```

Record in the final response that Arduino compilation was intentionally skipped until the ESP32 core is available.

- [ ] **Step 6: Commit final verification note only if files changed**

If Task 5 produced no file changes, do not create an empty commit.

---

## Self-Review

- Spec coverage: The plan covers default Clawd fallback, `OFFLINE`, Codex online `IDLE`, core-pulse drawing, state colors, phase bars, watcher lifecycle mapping, docs, and non-Arduino verification.
- Scope: The plan intentionally avoids WiFi, OTA, Web page structure, and Arduino compilation.
- Compatibility: Existing `PLAN / CODE / TEST / DONE / BLOCK`, `/progress`, `/state`, and USB `PROGRESS` remain compatible. `OFFLINE` is additive.
- Test path: Static contract test catches missing firmware helpers, script lifecycle strings, and firmware/dist drift.
