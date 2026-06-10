#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

LOG="$TMP/states.log"
STAGE="$TMP/stage.sh"
SESSIONS="$TMP/sessions"
mkdir -p "$SESSIONS"

cat >"$STAGE" <<'EOS'
#!/usr/bin/env bash
state=""
msg=""
while [ "$#" -gt 0 ]; do
  case "$1" in
    -State|--state)
      state="${2:-}"
      shift 2
      ;;
    -Message|--message|-Msg|--msg)
      msg="${2:-}"
      shift 2
      ;;
    *)
      shift
      ;;
  esac
done
printf '%s %s\n' "$state" "$msg" >>"$CODEX_WATCH_TEST_LOG"
EOS
chmod +x "$STAGE"

run_watch() {
  CODEX_WATCH_TEST_LOG="$LOG" "$ROOT/tools/codex-watch.sh" \
    --once \
    --stage-script "$STAGE" \
    --sessions-dir "$SESSIONS" \
    --done-after 20 \
    --idle-after 300 \
    --heartbeat 60
}

assert_log() {
  local expected="$1"
  local actual
  actual="$(cat "$LOG" 2>/dev/null || true)"
  if [ "$actual" != "$expected" ]; then
    printf 'Expected watcher states:\n%s\nActual watcher states:\n%s\n' "$expected" "$actual" >&2
    exit 1
  fi
}

run_watch
assert_log "OFFLINE codex-offline"

pid_file="$TMP/codex-watch.pid"
watch_log="$TMP/codex-watch.log"
: >"$LOG"
CODEX_WATCH_TEST_LOG="$LOG" "$ROOT/tools/codex-watch.sh" \
  --background \
  --stage-script "$STAGE" \
  --sessions-dir "$SESSIONS" \
  --interval 1 \
  --done-after 20 \
  --idle-after 300 \
  --heartbeat 60 \
  --pid-file "$pid_file" \
  --log-file "$watch_log"
"$ROOT/tools/codex-watch.sh" --status --pid-file "$pid_file" >/dev/null
"$ROOT/tools/codex-watch.sh" --stop --pid-file "$pid_file" >/dev/null
for _ in 1 2 3 4 5; do
  if ! "$ROOT/tools/codex-watch.sh" --status --pid-file "$pid_file" >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
if "$ROOT/tools/codex-watch.sh" --status --pid-file "$pid_file" >/dev/null 2>&1; then
  echo "Expected background watcher to stop" >&2
  exit 1
fi

: >"$LOG"
session="$SESSIONS/current.jsonl"
printf '{}\n' >"$session"
run_watch
assert_log "PLAN codex-session"

: >"$LOG"
claude_session="$SESSIONS/claude.jsonl"
printf '{}\n' >"$claude_session"
CODEX_WATCH_TEST_LOG="$LOG" SESSIONS_DIR="$SESSIONS" "$ROOT/tools/claude-watch.sh" \
  --once \
  --stage-script "$STAGE" \
  --done-after 20 \
  --idle-after 300 \
  --heartbeat 60
assert_log "PLAN claude-session"

: >"$LOG"
event_session="$SESSIONS/event.jsonl"
rm -f "$session" "$claude_session"
cat >"$event_session" <<'EOS'
{"timestamp":"2026-06-07T00:00:00Z","type":"event_msg","payload":{"type":"task_started"}}
{"timestamp":"2026-06-07T00:00:01Z","type":"event_msg","payload":{"type":"user_message"}}
EOS
CODEX_WATCH_TEST_LOG="$LOG" "$ROOT/tools/codex-watch.sh" \
  --stage-script "$STAGE" \
  --sessions-dir "$SESSIONS" \
  --interval 1 \
  --done-after 2 \
  --idle-after 60 \
  --heartbeat 60 \
  --pid-file "$pid_file" \
  --log-file "$watch_log" &
event_watch_pid="$!"
sleep 3
assert_log "PLAN codex-session"
printf '%s\n' '{"timestamp":"2026-06-07T00:00:03Z","type":"response_item","payload":{"type":"message"}}' >>"$event_session"
sleep 3
assert_log "PLAN codex-session
CODE active"
printf '%s\n' '{"timestamp":"2026-06-07T00:00:06Z","type":"event_msg","payload":{"type":"task_complete"}}' >>"$event_session"
for _ in 1 2 3 4 5; do
  if grep -Fq 'DONE turn-complete' "$LOG"; then
    break
  fi
  sleep 1
done
assert_log "PLAN codex-session
CODE active
DONE turn-complete"
kill "$event_watch_pid" >/dev/null 2>&1 || true
wait "$event_watch_pid" >/dev/null 2>&1 || true

: >"$LOG"
old="$SESSIONS/old.jsonl"
printf '{}\n' >"$old"
touch -d '10 minutes ago' "$old" 2>/dev/null || touch -t 202001010000 "$old"
rm -f "$session"
rm -f "$claude_session"
rm -f "$event_session"
run_watch
assert_log "OFFLINE codex-offline"

echo "Codex watcher behavior passed"
