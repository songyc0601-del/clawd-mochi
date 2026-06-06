# Codex Status Avatar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a Codex core-pulse status screen that appears only while Codex is online, and returns to the default Clawd animation when Codex is offline.

**Architecture:** Keep the existing firmware protocol compatible and add `OFFLINE` as a public status for “return to default view.” The firmware owns display semantics: `OFFLINE` draws the default Clawd animation without calling `markAction()`, while `IDLE / PLAN / CODE / TEST / DONE / BLOCK` draw the Codex core-pulse layer and update the Codex heartbeat. The client watcher owns lifecycle hints, re-sends the current meaningful state every 60 seconds, and sends `OFFLINE codex-offline` only when no Codex session exists or a long-running watcher exits.

**Tech Stack:** Arduino C++ for ESP32-C3, Adafruit ST7789 drawing primitives, Bash/PowerShell watcher scripts, repository static shell tests. Do not run Arduino compilation while the ESP32 core is still missing or downloading.

---

## Files

- Modify: `clawd_mochi/clawd_mochi.ino`
  - Add `OFFLINE` status handling.
  - Add 120 second device-side Codex heartbeat timeout.
  - Keep `OFFLINE` out of `markAction()` handling for HTTP and USB.
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
  - Add `--heartbeat SECONDS`, default 60.
  - Push `PLAN codex-session` for a new recent session, then `CODE active` only after a later mtime change.
  - Preserve `DONE turn-complete` until 300 seconds of inactivity.
  - Push `IDLE codex-ready` only when a long-running watcher still has a session but no active task remains.
  - Push `OFFLINE codex-offline` when no session file exists or when the long-running watcher exits.
  - Do not send exit `OFFLINE` in `--once` mode.
- Modify: `tools/codex-watch.ps1`
  - Mirror the watcher lifecycle behavior for Windows.
- Modify: embedded Web page in `clawd_mochi/clawd_mochi.ino` and `dist/clawd_mochi/clawd_mochi.ino`
  - Display `OFFLINE` as `Codex 离线`, progress 0/4.
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
  'void drawDefaultClawdView()'
  'const uint32_t CODEX_OFFLINE_TIMEOUT_MS = 120000UL;'
  'uint32_t lastCodexProgressMs = 0;'
  'void checkCodexOfflineTimeout()'
  'codex-timeout'
  'normalized != PROGRESS_OFFLINE'
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
  'HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"'
  '--heartbeat'
  'push_state "PLAN" "codex-session"'
  'push_state "OFFLINE" "codex-offline"'
  'trap cleanup EXIT INT TERM'
  'if [ "$ONCE" = "0" ]'
  'send_heartbeat'
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
uint32_t lastCodexProgressMs = 0;

const uint32_t CODEX_OFFLINE_TIMEOUT_MS = 120000UL;
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

Use fixed color semantics: `IDLE` low-brightness gray, `PLAN` blue, `CODE` orange, `TEST` teal, `DONE` green, `BLOCK` red.

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

`drawProgressBars(0, col)` must render four dark segments and no `0/4` text. `BLOCK` uses stage `4` with red.

- [ ] **Step 5: Add default Clawd fallback helper**

Add this helper near `showNormal()` or before `drawProgressView()`:

```cpp
void drawDefaultClawdView() {
  termMode = false;
  currentView = VIEW_EYES_NORMAL;
  animNormalEyes();
}
```

Do not call `markAction()` from this helper. `OFFLINE` is a system status transition, not a user action.

- [ ] **Step 6: Replace `drawProgressView()`**

Replace the current `drawProgressView()` body with:

```cpp
void drawProgressView() {
  termMode = false;

  if (progressState == PROGRESS_OFFLINE) {
    drawDefaultClawdView();
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

- [ ] **Step 7: Update `setProgress()` and startup default**

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
  if (state != PROGRESS_OFFLINE) lastCodexProgressMs = millis();
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

- [ ] **Step 8: Add device-side Codex timeout**

Add:

```cpp
void checkCodexOfflineTimeout() {
  if (!isCodexLayerState(progressState)) return;
  if (millis() - lastCodexProgressMs < CODEX_OFFLINE_TIMEOUT_MS) return;
  setProgress(PROGRESS_OFFLINE, "codex-timeout");
}
```

Call `checkCodexOfflineTimeout();` from `loop()` before or after `progressTick()`. Do not call `markAction()` from the timeout path.

- [ ] **Step 9: Replace `progressTick()`**

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

- [ ] **Step 10: Update HTTP and USB progress entry points**

Replace `routeProgress()` so `OFFLINE` does not call `markAction()`:

```cpp
void routeProgress() {
  const String state = server.hasArg("state") ? server.arg("state") : "";
  const String msg = server.hasArg("msg") ? server.arg("msg") : "";

  String normalized = state;
  normalized.trim();
  normalized.toUpperCase();
  if (normalized != PROGRESS_OFFLINE) markAction();

  if (!setProgress(state, msg)) {
    server.send(400, "application/json", "{\"e\":1}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}
```

In `handleSerialCommand()`, replace the `PROGRESS` branch with:

```cpp
  if (upper.startsWith("PROGRESS ")) {
    const int firstSpace = line.indexOf(' ', 9);
    String state = firstSpace < 0 ? line.substring(9) : line.substring(9, firstSpace);
    String msg = firstSpace < 0 ? "" : line.substring(firstSpace + 1);

    String normalized = state;
    normalized.trim();
    normalized.toUpperCase();
    if (normalized != PROGRESS_OFFLINE) markAction();

    if (setProgress(state, msg)) return "OK";
    return "ERR bad-state";
  }
```

- [ ] **Step 11: Update embedded Web page state labels**

In the compact Web page JavaScript labels, add:

```js
OFFLINE:'Codex 离线'
```

In the steps mapping, add:

```js
OFFLINE:0
```

Ensure `OFFLINE` is not shown as `待机中`.

- [ ] **Step 12: Mirror firmware file**

Run:

```bash
cp clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
```

- [ ] **Step 13: Run static test**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected:

```text
Codex status avatar contract passed
```

- [ ] **Step 14: Commit firmware display changes**

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

- [ ] **Step 1: Update shell stage help and watcher options**

In `tools/codex-stage.sh`, replace:

```text
States: IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE
```

with:

```text
States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE
```

In `tools/codex-watch.sh`, add the default:

```bash
HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"
```

Add option parsing:

```bash
    --heartbeat)
      HEARTBEAT_SECONDS="${2:-}"
      shift 2
      ;;
```

Update usage:

```text
  --heartbeat SECONDS   Re-send current online state. Default: 60
```

Update environment list to include:

```text
HEARTBEAT_SECONDS
```

- [ ] **Step 2: Update PowerShell stage validation**

In `tools/codex-stage.ps1`, replace the `ValidateSet` with:

```powershell
[ValidateSet("OFFLINE", "IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK", "STATE")]
```

- [ ] **Step 3: Add shell watcher cleanup that skips `--once`**

After variable initialization in `tools/codex-watch.sh`, add:

```bash
cleanup() {
  if [ "$ONCE" = "0" ]; then
    push_state "OFFLINE" "codex-offline"
  fi
}

trap cleanup EXIT INT TERM
```

- [ ] **Step 4: Add shell watcher heartbeat helper**

Add:

```bash
last_heartbeat=0

send_heartbeat() {
  local now="$1"
  if [ "$current_state" = "OFFLINE" ] || [ -z "$current_state" ]; then
    return 0
  fi
  if [ $((now - last_heartbeat)) -lt "$HEARTBEAT_SECONDS" ]; then
    return 0
  fi

  case "$current_state" in
    PLAN)  push_state "PLAN" "codex-session" ;;
    CODE)  push_state "CODE" "active" ;;
    DONE)  push_state "DONE" "turn-complete" ;;
    BLOCK) push_state "BLOCK" "need-input" ;;
    IDLE)  push_state "IDLE" "codex-ready" ;;
  esac
  last_heartbeat="$now"
}
```

- [ ] **Step 5: Change no-session handling in shell watcher**

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

- [ ] **Step 6: Change new-session age handling in shell watcher**

In the new-session branch, implement this exact age behavior:

```bash
    if [ "$age" -le "$DONE_AFTER_SECONDS" ]; then
      push_state "PLAN" "codex-session"
      current_state="PLAN"
      last_heartbeat="$now"
    elif [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
      push_state "DONE" "turn-complete"
      current_state="DONE"
      done_sent=1
      last_heartbeat="$now"
    else
      push_state "OFFLINE" "codex-offline"
      current_state="OFFLINE"
      done_sent=1
      idle_sent=1
    fi
```

Do not immediately push `CODE active` after `PLAN`. `CODE` is only sent after a later mtime change.

- [ ] **Step 7: Change mtime-change handling in shell watcher**

When the same latest session file changes, keep the existing `CODE active` transition:

```bash
  if [ "$mtime" != "$last_mtime" ]; then
    last_mtime="$mtime"
    last_activity="$mtime"
    done_sent=0
    idle_sent=0
    if [ "$current_state" != "CODE" ]; then
      push_state "CODE" "active"
      current_state="CODE"
      last_heartbeat="$now"
    fi
    return 0
  fi
```

- [ ] **Step 8: Change long inactivity handling in shell watcher**

Use this order after mtime checks:

```bash
  age=$((now - last_activity))

  if [ "$age" -ge "$IDLE_AFTER_SECONDS" ] && [ "$idle_sent" = "0" ] && [ "$current_state" != "BLOCK" ]; then
    push_state "IDLE" "codex-ready"
    current_state="IDLE"
    idle_sent=1
    last_heartbeat="$now"
    return 0
  fi

  if [ "$age" -ge "$DONE_AFTER_SECONDS" ] && [ "$done_sent" = "0" ] && [ "$current_state" != "BLOCK" ]; then
    push_state "DONE" "turn-complete"
    current_state="DONE"
    done_sent=1
    last_heartbeat="$now"
    return 0
  fi

  send_heartbeat "$now"
```

This preserves `DONE` until 300 seconds, never automatically downgrades `BLOCK`, and re-sends the current state every heartbeat interval.

- [ ] **Step 9: Update Windows watcher validation and behavior**

In `tools/codex-watch.ps1`, set the state validator to:

```powershell
[ValidateSet("OFFLINE", "IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK")]
```

Add parameter:

```powershell
[int]$HeartbeatSeconds = 60
```

Register cleanup, skipping `-Once`:

```powershell
Register-EngineEvent PowerShell.Exiting -Action {
  if (-not $Once) {
    try {
      & $StageScript -DeviceUrl $DeviceUrl -State OFFLINE -Message "codex-offline" | Out-Null
    } catch {}
  }
} | Out-Null
```

Mirror shell behavior:

```powershell
# No session:
Push-State -State OFFLINE -Message "codex-offline"
$script:currentState = "OFFLINE"

# New recent session:
Push-State -State PLAN -Message "codex-session"
$script:currentState = "PLAN"

# Later mtime change:
Push-State -State CODE -Message "active"
$script:currentState = "CODE"

# DONE threshold:
Push-State -State DONE -Message "turn-complete"
$script:currentState = "DONE"

# IDLE threshold, unless current state is BLOCK:
Push-State -State IDLE -Message "codex-ready"
$script:currentState = "IDLE"
```

- [ ] **Step 10: Run script syntax checks**

Run:

```bash
bash -n tools/codex-stage.sh
bash -n tools/codex-watch.sh
./tools/codex-watch.sh --once --verbose --stage-script /bin/true
./tools/codex-watch.sh --once --verbose --stage-script /bin/true --heartbeat 10
./tools/test-codex-status-avatar.sh
```

Expected:

```text
[codex-watch] push ...
Codex status avatar contract passed
```

PowerShell scripts are not run in WSL unless `pwsh` or Windows PowerShell is available. Confirm by inspection that `-Once` cleanup does not send `OFFLINE`.

- [ ] **Step 11: Commit script changes**

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
- `/state` 会返回 `progress:"OFFLINE"`，`progressMsg` 保留 `codex-offline` 或 `codex-timeout` 等离线原因。
- Web 控制页显示 `OFFLINE` 为“Codex 离线”，不能显示为“待机中”。
```

- [ ] **Step 2: Update client sync document**

In `docs/codex-client-status-sync.zh-CN.md`, update watcher behavior to:

```markdown
- watcher 启动时若最新 session 在 20 秒内：推送 `PLAN codex-session`。
- watcher 启动时若最新 session 在 20-300 秒内：推送 `DONE turn-complete`。
- watcher 启动时若最新 session 超过 300 秒，或未发现 session：推送 `OFFLINE codex-offline`。
- session 文件后续变化：推送 `CODE active`。
- 默认 20 秒无变化：推送 `DONE turn-complete`。
- 默认 300 秒无变化：推送 `IDLE codex-ready`。
- `DONE` 在 300 秒内通过心跳保持，不被心跳覆盖成 `IDLE`。
- `BLOCK` 不自动降级为 `DONE` 或 `IDLE`，只由明确新状态或离线超时替换。
- watcher 心跳默认 60 秒，可通过 `--heartbeat` 或 `HEARTBEAT_SECONDS` 配置。
- `--once` / `-Once` 退出时不发送 `OFFLINE`。
- 长期 watcher 退出、未发现 session 或 Codex 离线：推送 `OFFLINE codex-offline`，设备回到默认 Clawd 动画形象。
- 设备端 120 秒未收到非 `OFFLINE` 状态：自动回到 `OFFLINE codex-timeout`。
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
`PROGRESS OFFLINE ...` 不应被固件记录为用户操作；其他 `PROGRESS` 状态可以记录为活动。
```

- [ ] **Step 4: Run documentation/static checks**

Run:

```bash
./tools/test-codex-status-avatar.sh
rg -n "OFFLINE|codex-ready|codex-offline|codex-timeout|heartbeat|心跳|Codex 离线" README.zh-CN.md docs/codex-client-status-sync.zh-CN.md docs/usb-serial-codex.zh-CN.md
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
