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
  'if (state == PROGRESS_OFFLINE) return "codex-offline";'
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
  if ! grep -Fq -- "$text" "$FW"; then
    echo "Firmware missing Codex status avatar contract: $text" >&2
    exit 1
  fi
  if ! grep -Fq -- "$text" "$DIST"; then
    echo "Dist firmware missing Codex status avatar contract: $text" >&2
    exit 1
  fi
done

if ! grep -Fq -- 'States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE' "$STAGE_SH"; then
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
  if ! grep -Fq -- "$text" "$WATCH_SH"; then
    echo "codex-watch.sh missing lifecycle contract: $text" >&2
    exit 1
  fi
done

if ! cmp -s "$FW" "$DIST"; then
  echo "clawd_mochi and dist firmware must stay identical" >&2
  exit 1
fi

echo "Codex status avatar contract passed"
