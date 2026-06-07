#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

LOG="$TMP/states.log"
STAGE="$TMP/stage.sh"
CODEX_SESSIONS="$TMP/codex"
CLAUDE_SESSIONS="$TMP/claude"
PID_FILE="$TMP/agent-watch.pid"
WATCH_LOG="$TMP/agent-watch.log"
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

: >"$LOG"
AGENT_WATCH_TEST_LOG="$LOG" "$ROOT/tools/agent-watch.sh" \
  --background \
  --stage-script "$STAGE" \
  --codex-sessions-dir "$CODEX_SESSIONS" \
  --claude-sessions-dir "$CLAUDE_SESSIONS" \
  --mode auto \
  --interval 1 \
  --done-after 20 \
  --idle-after 300 \
  --heartbeat 1 \
  --pid-file "$PID_FILE" \
  --log-file "$WATCH_LOG" >/dev/null
"$ROOT/tools/agent-watch.sh" --status --pid-file "$PID_FILE" >/dev/null
sleep 2
"$ROOT/tools/agent-watch.sh" --stop --pid-file "$PID_FILE" >/dev/null
for _ in 1 2 3 4 5; do
  if ! "$ROOT/tools/agent-watch.sh" --status --pid-file "$PID_FILE" >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
if "$ROOT/tools/agent-watch.sh" --status --pid-file "$PID_FILE" >/dev/null 2>&1; then
  echo "Expected background agent watcher to stop" >&2
  exit 1
fi
if ! grep -Fq 'none OFFLINE agents-offline' "$LOG"; then
  echo "Expected background watcher to push agents-offline on exit" >&2
  exit 1
fi

echo "Agent watcher behavior passed"
