#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
FW="$ROOT/clawd_mochi/clawd_mochi.ino"
STAGE_SH="$ROOT/tools/codex-stage.sh"
WATCH_SH="$ROOT/tools/codex-watch.sh"
AGENT_SH="$ROOT/tools/agent-watch.sh"

for file in "$FW" "$STAGE_SH" "$WATCH_SH" "$AGENT_SH"; do
  if [ ! -f "$file" ]; then
    echo "Missing required file: $file" >&2
    exit 1
  fi
done

required_fw=(
  'const String PROGRESS_OFFLINE = "OFFLINE";'
  'bool isCodexLayerState(const String& state)'
  'void drawCodexCore(uint16_t col, uint8_t pulse)'
  'void drawCodexProgressView()'
  'void drawClaudeCodeStyleLayer(uint16_t col, uint8_t pulse)'
  'void drawClaudeProgressView()'
  'void drawProgressBars(uint8_t stage, uint16_t col)'
  'String progressDefaultMessage(const String& state)'
  'if (state == PROGRESS_OFFLINE) return "codex-offline";'
  'if (state == PROGRESS_OFFLINE)'
  'if (progressSource == "claude")'
  'tft.print("CLAUDE");'
  'Claude Code Style Layer'
  'void drawDefaultClawdView()'
  'const uint32_t CODEX_OFFLINE_TIMEOUT_MS = 120000UL;'
  'uint32_t lastCodexProgressMs = 0;'
  'String   progressSource = "none";'
  'String   agentMode = "AUTO";'
  'bool isProgressSource(const String& source)'
  'bool setAgentMode(String mode)'
  'void routeAgentMode()'
  'void checkCodexOfflineTimeout()'
  'codex-timeout'
  'server.hasArg("source")'
  'progressSource = source'
  'source != "codex" && source != "claude" && source != "none"'
  'server.on("/agent-mode",  HTTP_GET, routeAgentMode);'
  'j += ",\"progressSource\":\"";'
  'j += ",\"agentMode\":\"";'
  'Auto | Codex | Claude'
  '/agent-mode?mode='
  'normalized != PROGRESS_OFFLINE'
  'progressPulsePhase'
  'Codex core-pulse status layer'
)

for text in "${required_fw[@]}"; do
  if ! grep -Fq -- "$text" "$FW"; then
    echo "Firmware missing Codex status avatar contract: $text" >&2
    exit 1
  fi
done

if ! grep -Fq -- 'States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE' "$STAGE_SH"; then
  echo "codex-stage.sh help must list OFFLINE" >&2
  exit 1
fi

if ! grep -Fq -- '-Source|--source' "$STAGE_SH"; then
  echo "codex-stage.sh must accept optional source" >&2
  exit 1
fi

if ! grep -Fq -- 'source=' "$STAGE_SH"; then
  echo "codex-stage.sh must forward source to /progress" >&2
  exit 1
fi

required_watch=(
  'HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"'
  '--heartbeat'
  'CLIENT_SLUG="${CLIENT_SLUG:-codex}"'
  'push_state "PLAN" "$(online_message session)"'
  'push_state "OFFLINE" "$(online_message offline)"'
  'trap cleanup EXIT'
  'trap handle_signal INT TERM'
  'if [ "$ONCE" = "0" ]'
  'send_heartbeat'
)

for text in "${required_watch[@]}"; do
  if ! grep -Fq -- "$text" "$WATCH_SH"; then
    echo "codex-watch.sh missing lifecycle contract: $text" >&2
    exit 1
  fi
done

required_agent_watch=(
  'tools/agent-watch.sh [options]'
  '--codex-sessions-dir'
  '--claude-sessions-dir'
  '--mode MODE'
  'read_device_mode'
  'LAST_KNOWN_MODE'
  'select_status'
  'state_rank'
  'BLOCK) printf'
  'push_state "none" "OFFLINE" "agents-offline"'
  '-Source "$source"'
)

for text in "${required_agent_watch[@]}"; do
  if ! grep -Fq -- "$text" "$AGENT_SH"; then
    echo "agent-watch.sh missing unified watcher contract: $text" >&2
    exit 1
  fi
done

echo "Codex status avatar contract passed"
