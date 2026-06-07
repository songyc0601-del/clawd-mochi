# Agent Display Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a unified Codex / Claude Code status watcher and Web-controlled display mode so one device can show either Codex core-pulse, Claude Code Style Layer, or default Clawd fallback without competing watcher updates.

**Architecture:** Keep the firmware protocol backward compatible by extending `/progress` with optional explicit `source=codex|claude|none`, adding `/agent-mode` for Web-controlled mode, and returning `agentMode` plus `progressSource` from `/state`. Add `tools/agent-watch.sh` as the only supported multi-client entry point; it observes Codex and Claude sessions, reads device mode by default, chooses one final status, and sends one `/progress` update. Existing `codex-watch` and `claude-watch` remain only for compatibility, internal reuse, or single-client troubleshooting.

**Tech Stack:** Arduino C++ for ESP32-C3, embedded HTML/JS, Bash watcher scripts, PowerShell stage compatibility, static shell tests. Run Arduino compile only after script/static tests pass.

---

## Files

- Create: `tools/agent-watch.sh`
  - Unified WSL/Linux/macOS watcher for Codex and Claude Code.
  - Supports `--mode device|auto|codex|claude`, default `device`.
  - Supports `--background`, `--stop`, `--status`, `--once`.
- Create: `tools/test-agent-watch-behavior.sh`
  - Public CLI behavior tests for unified watcher source selection.
- Modify: `tools/codex-stage.sh`
  - Add optional `-Source|--source`.
  - Append `source` to `/progress` only when provided.
- Modify: `tools/codex-stage.ps1`
  - Add optional `-Source`.
  - Preserve USB fallback behavior; HTTP sends `source` when provided.
- Modify: `tools/test-codex-status-avatar.sh`
  - Add static contract checks for `progressSource`, `agentMode`, `/agent-mode`, `source=`.
- Modify: `tools/test-codex-watch-behavior.sh`
  - Keep existing single-client watcher coverage green.
- Modify: `clawd_mochi/clawd_mochi.ino`
  - Add `progressSource` and `agentMode`.
  - Parse `/progress?source=...`.
  - Add `/agent-mode`.
  - Return new fields from `/state`.
  - Add Web mode selector.
  - Add Claude Code display branch.
- Modify: `dist/clawd_mochi/clawd_mochi.ino`
  - Mirror firmware changes exactly.
- Modify: `README.zh-CN.md`, `README.md`
  - Document multi-client watcher and Web display mode.
- Modify: `docs/codex-client-status-sync.zh-CN.md`
  - Document unified watcher and source protocol.
- Modify: `docs/claude-code-status-sync.zh-CN.md`
  - Document Claude Code participation in unified watcher.
- Modify: `docs/user-manual.zh-CN.md`
  - Add user steps for Web mode and unified watcher.

---

### Task 1: Add Unified Watcher Behavior Test

**Files:**
- Create: `tools/test-agent-watch-behavior.sh`

- [ ] **Step 1: Write the failing test**

Create `tools/test-agent-watch-behavior.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

LOG="$TMP/states.log"
STAGE="$TMP/stage.sh"
CODEX_SESSIONS="$TMP/codex"
CLAUDE_SESSIONS="$TMP/claude"
mkdir -p "$CODEX_SESSIONS" "$CLAUDE_SESSIONS"

cat >"$STAGE" <<'EOS'
#!/usr/bin/env bash
state=""
msg=""
source=""
while [ "$#" -gt 0 ]; do
  case "$1" in
    -State|--state) state="${2:-}"; shift 2 ;;
    -Message|--message|-Msg|--msg) msg="${2:-}"; shift 2 ;;
    -Source|--source) source="${2:-}"; shift 2 ;;
    *) shift ;;
  esac
done
printf '%s %s %s\n' "$source" "$state" "$msg" >>"$AGENT_WATCH_TEST_LOG"
EOS
chmod +x "$STAGE"

run_agent_watch() {
  AGENT_WATCH_TEST_LOG="$LOG" "$ROOT/tools/agent-watch.sh" \
    --once \
    --stage-script "$STAGE" \
    --codex-sessions-dir "$CODEX_SESSIONS" \
    --claude-sessions-dir "$CLAUDE_SESSIONS" \
    --mode "$1" \
    --done-after 20 \
    --idle-after 300 \
    --heartbeat 60
}

assert_log() {
  local expected="$1"
  local actual
  actual="$(cat "$LOG" 2>/dev/null || true)"
  if [ "$actual" != "$expected" ]; then
    printf 'Expected agent watcher states:\n%s\nActual agent watcher states:\n%s\n' "$expected" "$actual" >&2
    exit 1
  fi
}

run_agent_watch auto
assert_log "none OFFLINE agents-offline"

: >"$LOG"
printf '{}\n' >"$CODEX_SESSIONS/codex.jsonl"
run_agent_watch auto
assert_log "codex PLAN codex-session"

: >"$LOG"
printf '{}\n' >"$CLAUDE_SESSIONS/claude.jsonl"
touch "$CODEX_SESSIONS/codex.jsonl"
sleep 1
touch "$CLAUDE_SESSIONS/claude.jsonl"
run_agent_watch auto
assert_log "claude PLAN claude-session"

: >"$LOG"
run_agent_watch codex
assert_log "codex PLAN codex-session"

: >"$LOG"
run_agent_watch claude
assert_log "claude PLAN claude-session"

: >"$LOG"
rm -f "$CLAUDE_SESSIONS/claude.jsonl"
run_agent_watch claude
assert_log "none OFFLINE claude-offline"

echo "Agent watcher behavior passed"
```

- [ ] **Step 2: Make the test executable**

Run:

```bash
chmod +x tools/test-agent-watch-behavior.sh
```

- [ ] **Step 3: Run the test to verify RED**

Run:

```bash
./tools/test-agent-watch-behavior.sh
```

Expected: FAIL because `tools/agent-watch.sh` does not exist.

---

### Task 2: Add Source Support to Stage Scripts

**Files:**
- Modify: `tools/codex-stage.sh`
- Modify: `tools/codex-stage.ps1`

- [ ] **Step 1: Add shell stage source test to the static contract**

Append these checks to `tools/test-codex-status-avatar.sh` near the stage script assertions:

```bash
if ! grep -Fq -- '-Source|--source' "$STAGE_SH"; then
  echo "codex-stage.sh must accept optional source" >&2
  exit 1
fi

if ! grep -Fq -- 'source=' "$STAGE_SH"; then
  echo "codex-stage.sh must forward source to /progress" >&2
  exit 1
fi
```

- [ ] **Step 2: Run contract test to verify RED**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected: FAIL with `codex-stage.sh must accept optional source`.

- [ ] **Step 3: Implement shell source argument**

In `tools/codex-stage.sh`, add:

```bash
SOURCE=""
```

Add option parsing:

```bash
    -Source|--source)
      SOURCE="${2:-}"
      shift 2
      ;;
```

Build the `/progress` query so `source` is optional:

```bash
args=(-G --data-urlencode "state=$STATE" --data-urlencode "msg=$MESSAGE")
if [ -n "$SOURCE" ]; then
  args+=(--data-urlencode "source=$SOURCE")
fi

curl -fsS "${args[@]}" "$DEVICE_URL/progress" >/dev/null
```

- [ ] **Step 4: Implement PowerShell source argument**

In `tools/codex-stage.ps1`, add parameter:

```powershell
[ValidateSet("", "codex", "claude", "none")]
[string]$Source = ""
```

Include it in the HTTP query only when present:

```powershell
$query = "state=$([uri]::EscapeDataString($State))&msg=$([uri]::EscapeDataString($cleanMessage))"
if ($Source) {
  $query += "&source=$([uri]::EscapeDataString($Source))"
}
```

- [ ] **Step 5: Verify contract**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected: PASS.

---

### Task 3: Implement Unified Agent Watcher

**Files:**
- Create: `tools/agent-watch.sh`
- Test: `tools/test-agent-watch-behavior.sh`

- [ ] **Step 1: Implement minimal one-shot watcher**

Create `tools/agent-watch.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
CLAUDE_HOME="${CLAUDE_HOME:-$HOME/.claude}"
CODEX_SESSIONS_DIR="${CODEX_SESSIONS_DIR:-$CODEX_HOME/sessions}"
CLAUDE_SESSIONS_DIR="${CLAUDE_SESSIONS_DIR:-$CLAUDE_HOME/projects}"
DEVICE_URL="${DEVICE_URL:-http://192.168.4.1}"
STAGE_SCRIPT="${STAGE_SCRIPT:-$SCRIPT_DIR/codex-stage.sh}"
MODE="${MODE:-device}"
INTERVAL_SECONDS="${INTERVAL_SECONDS:-2}"
DONE_AFTER_SECONDS="${DONE_AFTER_SECONDS:-20}"
IDLE_AFTER_SECONDS="${IDLE_AFTER_SECONDS:-300}"
HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"
ONCE=0
VERBOSE="${VERBOSE:-0}"
BACKGROUND=0
STOP=0
STATUS=0
PID_FILE="${PID_FILE:-${XDG_RUNTIME_DIR:-/tmp}/clawd-mochi-agent-watch.pid}"
LOG_FILE="${LOG_FILE:-${XDG_STATE_HOME:-$HOME/.local/state}/clawd-mochi/agent-watch.log}"
LAST_KNOWN_MODE=""

usage() {
  cat <<'EOF'
Usage:
  tools/agent-watch.sh [options]

Options:
  --codex-sessions-dir DIR    Codex sessions directory. Default: $CODEX_HOME/sessions
  --claude-sessions-dir DIR   Claude Code sessions directory. Default: $CLAUDE_HOME/projects
  --device-url URL            Clawd Mochi URL. Default: http://192.168.4.1
  --stage-script FILE         Status push script. Default: tools/codex-stage.sh
  --mode MODE                 device, auto, codex, or claude. Default: device
  --interval SECONDS          Poll interval. Default: 2
  --done-after SECONDS        Mark DONE after no session writes. Default: 20
  --idle-after SECONDS        Mark IDLE after longer inactivity. Default: 300
  --heartbeat SECONDS         Re-send current state interval. Default: 60
  --once                      Run one poll iteration.
  --background                Start watcher in the background.
  --stop                      Stop background watcher.
  --status                    Show background watcher status.
  --pid-file FILE             Background pid file.
  --log-file FILE             Background log file.
  -v, --verbose               Print decisions.
  -h, --help                  Show this help.
EOF
}
```

Then implement argument parsing, `mtime_epoch`, `latest_session_file`, `session_has_lifecycle_events`, and `session_is_complete` using the same lifecycle semantics as `tools/codex-watch.sh`. The unified watcher must retain background controls, heartbeat, `task_complete` event priority, mtime fallback only when lifecycle events are absent, long-running exit `OFFLINE`, and `--once` exit behavior.

- [ ] **Step 2: Implement status candidates and selection**

Add:

```bash
status_rank() {
  case "$1" in
    BLOCK) echo 6 ;;
    TEST) echo 5 ;;
    CODE) echo 4 ;;
    PLAN) echo 3 ;;
    DONE) echo 2 ;;
    IDLE) echo 1 ;;
    OFFLINE) echo 0 ;;
    *) echo 0 ;;
  esac
}

candidate_for() {
  local source="$1"
  local dir="$2"
  local session_msg="$3"
  local ready_msg="$4"
  local offline_msg="$5"
  local now latest mtime age has_lifecycle is_complete

  latest="$(latest_session_file "$dir" || true)"
  if [ -z "$latest" ]; then
    printf '%s OFFLINE %s 0\n' "$source" "$offline_msg"
    return 0
  fi

  now="$(date +%s)"
  mtime="$(mtime_epoch "$latest")"
  age=$((now - mtime))
  has_lifecycle=0
  is_complete=0
  if session_has_lifecycle_events "$latest"; then
    has_lifecycle=1
    if session_is_complete "$latest"; then
      is_complete=1
    fi
  fi

  if [ "$age" -ge "$IDLE_AFTER_SECONDS" ]; then
    printf '%s IDLE %s %s\n' "$source" "$ready_msg" "$mtime"
  elif [ "$has_lifecycle" = "1" ] && [ "$is_complete" = "1" ]; then
    printf '%s DONE turn-complete %s\n' "$source" "$mtime"
  elif [ "$age" -le "$DONE_AFTER_SECONDS" ]; then
    printf '%s PLAN %s %s\n' "$source" "$session_msg" "$mtime"
  elif [ "$has_lifecycle" = "0" ]; then
    printf '%s DONE turn-complete %s\n' "$source" "$mtime"
  else
    printf '%s CODE active %s\n' "$source" "$mtime"
  fi
}

choose_candidate() {
  local mode="$1"
  local codex="$2"
  local claude="$3"
  local c_source c_state c_msg c_mtime h_source h_state h_msg h_mtime
  read -r c_source c_state c_msg c_mtime <<<"$codex"
  read -r h_source h_state h_msg h_mtime <<<"$claude"

  case "$mode" in
    codex) printf '%s\n' "$codex"; return 0 ;;
    claude) printf '%s\n' "$claude"; return 0 ;;
  esac

  if [ "$c_state" = "OFFLINE" ] && [ "$h_state" = "OFFLINE" ]; then
    printf 'none OFFLINE agents-offline 0\n'
    return 0
  fi
  if [ "$c_state" = "OFFLINE" ]; then printf '%s\n' "$claude"; return 0; fi
  if [ "$h_state" = "OFFLINE" ]; then printf '%s\n' "$codex"; return 0; fi

  local c_rank h_rank
  c_rank="$(status_rank "$c_state")"
  h_rank="$(status_rank "$h_state")"
  if [ "$c_rank" -gt "$h_rank" ]; then printf '%s\n' "$codex"; return 0; fi
  if [ "$h_rank" -gt "$c_rank" ]; then printf '%s\n' "$claude"; return 0; fi
  if [ "$h_mtime" -gt "$c_mtime" ]; then printf '%s\n' "$claude"; return 0; fi
  printf '%s\n' "$codex"
}
```

- [ ] **Step 3: Implement mode reading and push**

Add:

```bash
read_device_mode() {
  if [ "$MODE" != "device" ]; then
    printf '%s\n' "$MODE"
    return 0
  fi
  local json
  json="$(curl -fsS "$DEVICE_URL/state" 2>/dev/null || true)"
  case "$json" in
    *'"agentMode":"CODEX"'*) LAST_KNOWN_MODE="codex"; printf 'codex\n' ;;
    *'"agentMode":"CLAUDE"'*) LAST_KNOWN_MODE="claude"; printf 'claude\n' ;;
    *'"agentMode":"AUTO"'*) LAST_KNOWN_MODE="auto"; printf 'auto\n' ;;
    *) printf '%s\n' "${LAST_KNOWN_MODE:-auto}" ;;
  esac
}

push_selected() {
  local source="$1"
  local state="$2"
  local msg="$3"
  "$STAGE_SCRIPT" -DeviceUrl "$DEVICE_URL" -State "$state" -Message "$msg" -Source "$source" >/dev/null 2>&1 || true
}
```

`poll_once()` should call `candidate_for` for Codex and Claude, call `choose_candidate`, then call `push_selected` immediately when the selected tuple changed. If the selected tuple did not change, it should still re-send it every `HEARTBEAT_SECONDS`.

- [ ] **Step 4: Implement background controls**

Copy the background control shape from `tools/codex-watch.sh`, but use:

```bash
PID_FILE="${PID_FILE:-${XDG_RUNTIME_DIR:-/tmp}/clawd-mochi-agent-watch.pid}"
LOG_FILE="${LOG_FILE:-${XDG_STATE_HOME:-$HOME/.local/state}/clawd-mochi/agent-watch.log}"
```

The long-running cleanup should push:

```bash
push_selected none OFFLINE agents-offline
```

`--once` must not push exit `OFFLINE`.

- [ ] **Step 5: Verify watcher behavior**

Run:

```bash
bash -n tools/agent-watch.sh tools/test-agent-watch-behavior.sh
./tools/test-agent-watch-behavior.sh
```

Expected:

```text
Agent watcher behavior passed
```

---

### Task 4: Add Firmware Static Contract for Source and Mode

**Files:**
- Modify: `tools/test-codex-status-avatar.sh`

- [ ] **Step 1: Add failing firmware contract checks**

Add these strings to the `required_fw` array:

```bash
'String progressSource = "none";'
'String agentMode = "AUTO";'
'bool isProgressSource(const String& source)'
'bool setAgentMode(String mode)'
'void routeAgentMode()'
'server.on("/agent-mode"'
'progressSource'
'agentMode'
'drawClaudeProgressView'
'source=claude'
'source=none'
'progressSource = source'
'Auto'
'Claude'
```

- [ ] **Step 2: Run static test to verify RED**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected: FAIL on the first missing firmware contract string.

---

### Task 5: Add Firmware Protocol Fields and Web Mode Control

**Files:**
- Modify: `clawd_mochi/clawd_mochi.ino`
- Modify: `dist/clawd_mochi/clawd_mochi.ino`
- Test: `tools/test-codex-status-avatar.sh`

- [ ] **Step 1: Add globals and validators**

Near progress globals add:

```cpp
String   progressSource = "none";
String   agentMode = "AUTO";
```

Add helpers:

```cpp
bool isProgressSource(const String& source) {
  return source == "codex" || source == "claude" || source == "none";
}

bool setAgentMode(String mode) {
  mode.trim();
  mode.toUpperCase();
  if (mode != "AUTO" && mode != "CODEX" && mode != "CLAUDE") return false;
  agentMode = mode;
  return true;
}
```

- [ ] **Step 2: Extend routeProgress**

In `routeProgress()`, parse source before `setProgress()`:

```cpp
if (server.hasArg("source")) {
  String source = server.arg("source");
  source.trim();
  source.toLowerCase();
  if (!isProgressSource(source)) {
    server.send(400, "application/json", "{\"e\":1}");
    return;
  }
  progressSource = source;
}
```

Keep the existing `OFFLINE` branch so `OFFLINE` does not call `markAction()`.

- [ ] **Step 3: Add routeAgentMode**

Add:

```cpp
void routeAgentMode() {
  markAction();
  const String mode = server.hasArg("mode") ? server.arg("mode") : "";
  if (!setAgentMode(mode)) {
    server.send(400, "application/json", "{\"e\":1}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}
```

Register it in `setup()`:

```cpp
server.on("/agent-mode", HTTP_GET, routeAgentMode);
```

- [ ] **Step 4: Extend stateJson**

Add:

```cpp
j += ",\"progressSource\":\""; j += jsonEscape(progressSource); j += "\"";
j += ",\"agentMode\":\""; j += jsonEscape(agentMode); j += "\"";
```

- [ ] **Step 5: Add Web mode controls**

In `INDEX_HTML_LITE`, add mode buttons under the work section:

```html
<div class="modes">
<button id="modeAUTO" onclick="agentMode('auto')">Auto</button>
<button id="modeCODEX" onclick="agentMode('codex')">Codex</button>
<button id="modeCLAUDE" onclick="agentMode('claude')">Claude</button>
</div>
```

Add compact CSS:

```css
.modes{display:grid;grid-template-columns:repeat(3,1fr);gap:5px;margin-top:12px}.modes button{border:1px solid #59625b;background:#303832;color:#fff;border-radius:6px;padding:7px 0;font-weight:800}.modes button.on{border-color:#efbc42;color:#efbc42}
```

Add JS:

```js
async function agentMode(m){try{await req('/agent-mode?mode='+m);toast('\u5df2\u66f4\u65b0');refresh()}catch(e){failed()}}
function paintMode(m){['AUTO','CODEX','CLAUDE'].forEach(x=>{const el=$('mode'+x);if(el)el.classList.toggle('on',x===(m||'AUTO'))})}
```

In `refresh()`, call:

```js
paintMode(j.agentMode||'AUTO');
$('source').textContent=(j.progressSource==='claude')?'Claude Code':(j.progressSource==='codex'?'Codex':'None');
```

The Web control must not call `/progress`; it only saves `/agent-mode` and refreshes the UI. `agentMode` is not persisted across device reboot.

- [ ] **Step 6: Mirror firmware**

Run:

```bash
cp clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
```

- [ ] **Step 7: Verify static contract**

Run:

```bash
./tools/test-codex-status-avatar.sh
```

Expected: PASS.

---

### Task 6: Add Claude Code Display Branch

**Files:**
- Modify: `clawd_mochi/clawd_mochi.ino`
- Modify: `dist/clawd_mochi/clawd_mochi.ino`
- Test: `tools/test-codex-status-avatar.sh`

- [ ] **Step 1: Refactor Codex draw path behind a named helper**

Rename the current non-offline body of `drawProgressView()` into:

```cpp
void drawCodexProgressView() {
  currentView = VIEW_PROGRESS;
  const uint16_t col = progressColor(progressState);
  const uint8_t stage = progressStage(progressState);
  String message = progressMsg.length() > 0 ? progressMsg : progressDefaultMessage(progressState);
  const uint8_t pulse = (progressState == PROGRESS_DONE) ? 1 : progressPulsePhase;
  // existing Codex drawing body stays here
}
```

- [ ] **Step 2: Add Claude drawing helper**

Add:

```cpp
void drawClaudeProgressView() {
  currentView = VIEW_PROGRESS;
  const uint16_t col = progressColor(progressState);
  const uint8_t stage = progressStage(progressState);
  String message = progressMsg.length() > 0 ? progressMsg : progressDefaultMessage(progressState);

  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 7, col);
  tft.fillRect(0, DISP_H - 7, DISP_W, 7, col);

  tft.setTextColor(C_MUTED); tft.setTextSize(1);
  tft.setCursor(14, 18); tft.print("CLAUDE");

  const int16_t cx = 120;
  const int16_t cy = 84;
  const uint8_t r = 30 + (progressState == PROGRESS_DONE ? 1 : progressPulsePhase);
  tft.drawCircle(cx, cy, r, col);
  tft.drawCircle(cx - 18, cy, 20, col);
  tft.drawCircle(cx + 18, cy, 20, col);
  tft.drawCircle(cx, cy - 18, 20, col);
  tft.drawCircle(cx, cy + 18, 20, col);
  tft.fillCircle(cx, cy, 8 + (progressPulsePhase % 2), col);

  drawProgressBars(stage, progressState == PROGRESS_BLOCK ? progressColor(PROGRESS_BLOCK) : col);

  tft.setTextColor(col); tft.setTextSize(4);
  int16_t x = (DISP_W - progressState.length() * 24) / 2;
  if (x < 0) x = 0;
  tft.setCursor(x, 172); tft.print(progressState);

  if (message.length() > 0) {
    tft.setTextColor(C_WHITE); tft.setTextSize(1);
    int16_t msgX = (DISP_W - message.length() * 6) / 2;
    if (msgX < 8) msgX = 8;
    tft.setCursor(msgX, 214); tft.print(message);
  }
}
```

- [ ] **Step 3: Dispatch by source**

Update `drawProgressView()`:

```cpp
void drawProgressView() {
  termMode = false;
  if (progressState == PROGRESS_OFFLINE) {
    drawDefaultClawdView();
    return;
  }
  if (progressSource == "claude") {
    drawClaudeProgressView();
    return;
  }
  drawCodexProgressView();
}
```

- [ ] **Step 4: Update progressTick source branch**

Keep existing Codex local tick behavior for `progressSource != "claude"`. For Claude, redraw the Claude layer at the same interval:

```cpp
if (progressSource == "claude") {
  drawClaudeProgressView();
  return;
}
```

Place this after `progressPulsePhase` changes and before Codex-specific `drawCodexCore()`.

- [ ] **Step 5: Mirror and verify**

Run:

```bash
cp clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
./tools/test-codex-status-avatar.sh
```

Expected: PASS.

---

### Task 7: Update Documentation

**Files:**
- Modify: `README.zh-CN.md`
- Modify: `README.md`
- Modify: `docs/codex-client-status-sync.zh-CN.md`
- Modify: `docs/claude-code-status-sync.zh-CN.md`
- Modify: `docs/user-manual.zh-CN.md`

- [ ] **Step 1: Update Chinese README**

Add under the work status section:

```markdown
多客户端同时使用时只启动统一 watcher：

```bash
./tools/agent-watch.sh --device-url http://设备局域网IP --background
```

Web 页面“工作状态”区域可以选择 `Auto / Codex / Claude`：

- `Auto`：先按状态重要性选择，再用最近活动时间打平。
- `Codex`：固定显示 Codex 核心脉冲形象。
- `Claude`：固定显示 Claude Code Style Layer。
```

- [ ] **Step 2: Update English README**

Add equivalent concise English documentation:

```markdown
When both Codex and Claude Code are used, run the unified watcher:

```bash
./tools/agent-watch.sh --device-url http://DEVICE_IP --background
```

The Web UI work-status mode supports `Auto`, `Codex`, and `Claude`.
```

- [ ] **Step 3: Update client sync docs**

In `docs/codex-client-status-sync.zh-CN.md`, add:

```markdown
如果同一台设备同时接入 Codex 和 Claude Code，只使用 `tools/agent-watch.sh` 作为用户入口。不要让两个单客户端 watcher 同时推送到设备。统一 watcher 会根据 Web 端 `agentMode`、状态优先级和最近活动时间统一选择最终状态。
```

In `docs/claude-code-status-sync.zh-CN.md`, replace the old warning about not running both watcher scripts with:

```markdown
多客户端场景只使用 `tools/agent-watch.sh`。`tools/claude-watch.sh` 保留用于兼容、内部复用或单客户端排错，不再作为多客户端用户入口推荐。
```

- [ ] **Step 4: Update user manual**

Add a user section:

```markdown
### 同时使用 Codex 和 Claude Code

1. 打开设备 Web 页面。
2. 在“工作状态”中选择 `Auto`、`Codex` 或 `Claude`。
3. 在电脑启动统一 watcher：

```bash
./tools/agent-watch.sh --device-url http://设备局域网IP --background
```

`Auto` 会先按状态重要性选择，再用最近活动时间打平；固定模式只显示被选中的客户端。设备重启后模式恢复为 `Auto`。
```

- [ ] **Step 5: Verify docs references**

Run:

```bash
rg -n "agent-watch|agentMode|progressSource|Auto|Claude" README.md README.zh-CN.md docs/codex-client-status-sync.zh-CN.md docs/claude-code-status-sync.zh-CN.md docs/user-manual.zh-CN.md
```

Expected: output includes all updated docs.

---

### Task 8: Full Verification

**Files:**
- All changed files

- [ ] **Step 1: Run shell syntax checks**

Run:

```bash
bash -n tools/codex-stage.sh tools/codex-watch.sh tools/claude-watch.sh tools/agent-watch.sh tools/test-codex-status-avatar.sh tools/test-codex-watch-behavior.sh tools/test-agent-watch-behavior.sh
```

Expected: no output, exit 0.

- [ ] **Step 2: Run watcher tests**

Run:

```bash
./tools/test-agent-watch-behavior.sh
./tools/test-codex-watch-behavior.sh
./tools/test-codex-status-avatar.sh
```

Expected:

```text
Agent watcher behavior passed
Codex watcher behavior passed
Codex status avatar contract passed
```

- [ ] **Step 3: Run Arduino compile**

Run:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 clawd_mochi
```

Expected: compile succeeds.

- [ ] **Step 4: Check firmware mirror and whitespace**

Run:

```bash
cmp -s clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino
git diff --check
```

Expected: both commands exit 0.

- [ ] **Step 5: Manual Web API smoke test on real device**

After flashing firmware, run:

```bash
curl "http://设备局域网IP/agent-mode?mode=claude"
curl "http://设备局域网IP/state"
curl "http://设备局域网IP/progress?state=CODE&msg=active&source=claude"
curl "http://设备局域网IP/progress?state=CODE&msg=active&source=codex"
curl "http://设备局域网IP/progress?state=OFFLINE&msg=agents-offline&source=none"
```

Expected:

- `/agent-mode` returns `{"ok":1}`.
- `/state` includes `"agentMode":"CLAUDE"` and `"progressSource"`.
- `source=claude` shows the Claude Code Style Layer.
- `source=codex` shows Codex core-pulse animation.
- `source=none` with `OFFLINE` shows the default Clawd layer.

---

## Commit Plan

Use focused commits:

```bash
git add tools/test-agent-watch-behavior.sh tools/agent-watch.sh tools/codex-stage.sh tools/codex-stage.ps1 tools/test-codex-status-avatar.sh
git commit -m "添加多客户端统一状态 watcher"

git add clawd_mochi/clawd_mochi.ino dist/clawd_mochi/clawd_mochi.ino tools/test-codex-status-avatar.sh
git commit -m "支持 Web 控制客户端显示模式"

git add README.md README.zh-CN.md docs/codex-client-status-sync.zh-CN.md docs/claude-code-status-sync.zh-CN.md docs/user-manual.zh-CN.md
git commit -m "更新多客户端状态显示说明"
```

Before every commit, run:

```bash
git status
git branch --show-current
```

Only stage files related to the current task. Existing unrelated uncommitted files must not be reverted.

---

## Self-Review

- Spec coverage: The plan covers Web mode control, unified watcher, source protocol, `/state` fields, Claude display branch, docs, and verification.
- Compatibility: Existing `/progress?state=...&msg=...` remains valid because `source` is optional; omitted `source` does not change the current `progressSource`.
- Scope: Windows unified watcher is explicitly deferred; existing PowerShell single-client scripts remain supported and `codex-stage.ps1` keeps HTTP source compatibility.
- Testing: The first executable slice is `tools/test-agent-watch-behavior.sh`; firmware coverage is static plus Arduino compile and real-device smoke tests.
