#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
SESSIONS_DIR="${SESSIONS_DIR:-$CODEX_HOME/sessions}"
DEVICE_URL="${DEVICE_URL:-http://192.168.4.1}"
STAGE_SCRIPT="${STAGE_SCRIPT:-$SCRIPT_DIR/codex-stage.sh}"
CLIENT_SLUG="${CLIENT_SLUG:-codex}"
INTERVAL_SECONDS="${INTERVAL_SECONDS:-2}"
DONE_AFTER_SECONDS="${DONE_AFTER_SECONDS:-20}"
IDLE_AFTER_SECONDS="${IDLE_AFTER_SECONDS:-300}"
HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"
ONCE=0
VERBOSE="${VERBOSE:-0}"
BACKGROUND=0
STOP=0
STATUS=0
PID_FILE="${PID_FILE:-${XDG_RUNTIME_DIR:-/tmp}/clawd-mochi-codex-watch.pid}"
LOG_FILE="${LOG_FILE:-${XDG_STATE_HOME:-$HOME/.local/state}/clawd-mochi/codex-watch.log}"

usage() {
  cat <<'EOF'
Usage:
  tools/codex-watch.sh [options]

Options:
  --sessions-dir DIR     Codex sessions directory. Default: $CODEX_HOME/sessions
  --device-url URL       Clawd Mochi URL. Default: http://192.168.4.1
  --stage-script FILE    Status push script. Default: tools/codex-stage.sh
  --client-slug NAME     Message prefix for pushed states. Default: codex
  --interval SECONDS     Poll interval. Default: 2
  --done-after SECONDS   Mark DONE after no session writes. Default: 20
  --idle-after SECONDS   Mark IDLE after longer inactivity. Default: 300
  --heartbeat SECONDS    Re-send current state interval. Default: 60
  --once                 Run one poll iteration, useful for testing.
  --background           Start watcher in the background and write a pid file.
  --stop                 Stop the background watcher from the pid file.
  --status               Show whether the background watcher is running.
  --pid-file FILE        Background pid file. Default: $XDG_RUNTIME_DIR/clawd-mochi-codex-watch.pid or /tmp/...
  --log-file FILE        Background log file. Default: $XDG_STATE_HOME/clawd-mochi/codex-watch.log
  -v, --verbose          Print watcher decisions.
  -h, --help             Show this help.

Environment:
  CODEX_HOME, SESSIONS_DIR, DEVICE_URL, STAGE_SCRIPT, CLIENT_SLUG,
  INTERVAL_SECONDS, DONE_AFTER_SECONDS, IDLE_AFTER_SECONDS,
  HEARTBEAT_SECONDS, PID_FILE, LOG_FILE, VERBOSE
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --sessions-dir)
      SESSIONS_DIR="${2:-}"
      shift 2
      ;;
    --device-url)
      DEVICE_URL="${2:-}"
      shift 2
      ;;
    --stage-script)
      STAGE_SCRIPT="${2:-}"
      shift 2
      ;;
    --client-slug)
      CLIENT_SLUG="${2:-}"
      shift 2
      ;;
    --interval)
      INTERVAL_SECONDS="${2:-}"
      shift 2
      ;;
    --done-after)
      DONE_AFTER_SECONDS="${2:-}"
      shift 2
      ;;
    --idle-after)
      IDLE_AFTER_SECONDS="${2:-}"
      shift 2
      ;;
    --heartbeat)
      HEARTBEAT_SECONDS="${2:-}"
      shift 2
      ;;
    --once)
      ONCE=1
      shift
      ;;
    --background)
      BACKGROUND=1
      shift
      ;;
    --stop)
      STOP=1
      shift
      ;;
    --status)
      STATUS=1
      shift
      ;;
    --pid-file)
      PID_FILE="${2:-}"
      shift 2
      ;;
    --log-file)
      LOG_FILE="${2:-}"
      shift 2
      ;;
    -v|--verbose)
      VERBOSE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

log() {
  if [ "$VERBOSE" = "1" ]; then
    printf '[codex-watch] %s\n' "$*" >&2
  fi
}

pid_is_running() {
  local pid="$1"
  [ -n "$pid" ] && kill -0 "$pid" >/dev/null 2>&1
}

read_pid_file() {
  if [ -f "$PID_FILE" ]; then
    sed -n '1p' "$PID_FILE"
  fi
}

start_background() {
  local existing_pid
  local child_args
  existing_pid="$(read_pid_file || true)"

  if pid_is_running "$existing_pid"; then
    printf 'codex-watch already running: pid %s\n' "$existing_pid"
    return 0
  fi

  if [ "$ONCE" = "1" ]; then
    echo "--background cannot be used with --once" >&2
    exit 2
  fi

  mkdir -p "$(dirname "$PID_FILE")" "$(dirname "$LOG_FILE")"

  child_args=(
    --sessions-dir "$SESSIONS_DIR"
    --device-url "$DEVICE_URL"
    --stage-script "$STAGE_SCRIPT"
    --client-slug "$CLIENT_SLUG"
    --interval "$INTERVAL_SECONDS"
    --done-after "$DONE_AFTER_SECONDS"
    --idle-after "$IDLE_AFTER_SECONDS"
    --heartbeat "$HEARTBEAT_SECONDS"
    --pid-file "$PID_FILE"
    --log-file "$LOG_FILE"
  )
  if [ "$VERBOSE" = "1" ]; then
    child_args+=(--verbose)
  fi

  nohup "$0" "${child_args[@]}" >>"$LOG_FILE" 2>&1 &
  printf '%s\n' "$!" >"$PID_FILE"
  printf 'codex-watch started: pid %s, log %s\n' "$!" "$LOG_FILE"
}

stop_background() {
  local pid
  pid="$(read_pid_file || true)"
  if ! pid_is_running "$pid"; then
    rm -f "$PID_FILE"
    echo "codex-watch is not running"
    return 0
  fi

  kill "$pid"
  echo "codex-watch stopping"
}

show_status() {
  local pid
  pid="$(read_pid_file || true)"
  if pid_is_running "$pid"; then
    printf 'codex-watch running: pid %s\n' "$pid"
  else
    printf 'codex-watch not running\n'
    return 1
  fi
}

if [ "$STOP" = "1" ]; then
  stop_background
  exit 0
fi

if [ "$STATUS" = "1" ]; then
  show_status
  exit $?
fi

if [ "$BACKGROUND" = "1" ]; then
  start_background
  exit 0
fi

mtime_epoch() {
  if stat -c %Y "$1" >/dev/null 2>&1; then
    stat -c %Y "$1"
  else
    stat -f %m "$1"
  fi
}

latest_session_file() {
  local best=""
  local best_mtime=0
  local file
  local mtime

  if [ ! -d "$SESSIONS_DIR" ]; then
    return 1
  fi

  while IFS= read -r -d '' file; do
    mtime="$(mtime_epoch "$file" 2>/dev/null || printf '0')"
    if [ "$mtime" -gt "$best_mtime" ]; then
      best="$file"
      best_mtime="$mtime"
    fi
  done < <(find "$SESSIONS_DIR" -type f -name '*.jsonl' -print0 2>/dev/null)

  if [ -n "$best" ]; then
    printf '%s\n' "$best"
    return 0
  fi

  return 1
}

session_has_lifecycle_events() {
  local file="$1"
  grep -Eq '"type":"(task_started|task_complete|user_message)"' "$file" 2>/dev/null
}

session_is_complete() {
  local file="$1"
  awk '
    /"type":"task_started"/ || /"type":"user_message"/ { started = NR }
    /"type":"task_complete"/ { complete = NR }
    END { exit !(complete > started) }
  ' "$file" 2>/dev/null
}

last_file=""
last_mtime=""
last_activity=0
current_state=""
current_message=""
last_heartbeat=0
done_sent=0
idle_sent=0
cleanup_done=0

online_message() {
  local kind="$1"
  case "$kind" in
    session) printf '%s-session\n' "$CLIENT_SLUG" ;;
    ready) printf '%s-ready\n' "$CLIENT_SLUG" ;;
    offline) printf '%s-offline\n' "$CLIENT_SLUG" ;;
    *) printf '%s\n' "$kind" ;;
  esac
}

push_state() {
  local state="$1"
  local message="$2"

  log "push $state $message"
  "$STAGE_SCRIPT" -DeviceUrl "$DEVICE_URL" -State "$state" -Message "$message" >/dev/null 2>&1 || true
  current_state="$state"
  current_message="$message"
  last_heartbeat="$(date +%s)"
}

send_heartbeat() {
  local now="$1"
  if [ -z "$current_state" ]; then
    return 0
  fi
  if [ "$((now - last_heartbeat))" -lt "$HEARTBEAT_SECONDS" ]; then
    return 0
  fi
  push_state "$current_state" "$current_message"
}

cleanup() {
  local pid
  if [ "$cleanup_done" = "1" ]; then
    return 0
  fi
  cleanup_done=1

  if [ "$ONCE" = "0" ]; then
    push_state "OFFLINE" "$(online_message offline)"
  fi

  pid="$(read_pid_file || true)"
  if [ "$pid" = "$$" ]; then
    rm -f "$PID_FILE"
  fi
}

handle_signal() {
  cleanup
  exit 130
}

trap cleanup EXIT
trap handle_signal INT TERM

poll_once() {
  local now latest mtime age has_lifecycle is_complete
  now="$(date +%s)"
  latest="$(latest_session_file || true)"

  if [ -z "$latest" ]; then
    last_file=""
    last_mtime=""
    last_activity=0
    done_sent=0
    idle_sent=0
    if [ "$current_state" != "OFFLINE" ]; then
      push_state "OFFLINE" "$(online_message offline)"
    fi
    return 0
  fi

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

  if [ "$latest" != "$last_file" ]; then
    last_file="$latest"
    last_mtime="$mtime"
    last_activity="$mtime"
    done_sent=0
    idle_sent=0

    if [ "$has_lifecycle" = "1" ] && [ "$is_complete" = "1" ] && [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
      push_state "DONE" "turn-complete"
      done_sent=1
    elif [ "$age" -le "$DONE_AFTER_SECONDS" ]; then
      push_state "PLAN" "$(online_message session)"
    elif [ "$has_lifecycle" = "0" ] && [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
      push_state "DONE" "turn-complete"
      done_sent=1
    elif [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
      push_state "CODE" "active"
    else
      push_state "OFFLINE" "$(online_message offline)"
      done_sent=1
      idle_sent=1
    fi
    return 0
  fi

  if [ "$mtime" != "$last_mtime" ]; then
    last_mtime="$mtime"
    last_activity="$mtime"
    done_sent=0
    idle_sent=0
    if [ "$has_lifecycle" = "1" ] && [ "$is_complete" = "1" ]; then
      push_state "DONE" "turn-complete"
      done_sent=1
      return 0
    fi

    if [ "$current_state" != "CODE" ]; then
      push_state "CODE" "active"
    fi
    return 0
  fi

  age=$((now - last_activity))
  if [ "$current_state" != "BLOCK" ] && [ "$age" -ge "$IDLE_AFTER_SECONDS" ] && [ "$idle_sent" = "0" ]; then
    push_state "IDLE" "$(online_message ready)"
    idle_sent=1
    return 0
  fi

  if [ "$current_state" != "BLOCK" ] && [ "$has_lifecycle" = "1" ] && [ "$is_complete" = "1" ] && [ "$done_sent" = "0" ]; then
    push_state "DONE" "turn-complete"
    done_sent=1
    return 0
  fi

  if [ "$current_state" != "BLOCK" ] && [ "$has_lifecycle" = "0" ] && [ "$age" -ge "$DONE_AFTER_SECONDS" ] && [ "$done_sent" = "0" ]; then
    push_state "DONE" "turn-complete"
    done_sent=1
    return 0
  fi

  send_heartbeat "$now"
}

while true; do
  poll_once
  if [ "$ONCE" = "1" ]; then
    break
  fi
  sleep "$INTERVAL_SECONDS"
done
