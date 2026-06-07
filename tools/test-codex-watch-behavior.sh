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

: >"$LOG"
session="$SESSIONS/current.jsonl"
printf '{}\n' >"$session"
run_watch
assert_log "PLAN codex-session"

: >"$LOG"
old="$SESSIONS/old.jsonl"
printf '{}\n' >"$old"
touch -d '10 minutes ago' "$old" 2>/dev/null || touch -t 202001010000 "$old"
rm -f "$session"
run_watch
assert_log "OFFLINE codex-offline"

echo "Codex watcher behavior passed"
