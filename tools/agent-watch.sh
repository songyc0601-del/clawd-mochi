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
  --heartbeat SECONDS         Re-send current final state interval. Default: 60
  --once                      Run one poll iteration, useful for testing.
  --background                Start watcher in the background and write a pid file.
  --stop                      Stop the background watcher from the pid file.
  --status                    Show whether the background watcher is running.
  --pid-file FILE             Background pid file.
  --log-file FILE             Background log file.
  -v, --verbose               Print watcher decisions.
  -h, --help                  Show this help.

Environment:
  CODEX_HOME, CLAUDE_HOME, CODEX_SESSIONS_DIR, CLAUDE_SESSIONS_DIR,
  DEVICE_URL, STAGE_SCRIPT, MODE, INTERVAL_SECONDS, DONE_AFTER_SECONDS,
  IDLE_AFTER_SECONDS, HEARTBEAT_SECONDS, PID_FILE, LOG_FILE, VERBOSE
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --codex-sessions-dir) CODEX_SESSIONS_DIR="${2:-}"; shift 2 ;;
    --claude-sessions-dir) CLAUDE_SESSIONS_DIR="${2:-}"; shift 2 ;;
    --device-url) DEVICE_URL="${2:-}"; shift 2 ;;
    --stage-script) STAGE_SCRIPT="${2:-}"; shift 2 ;;
    --mode) MODE="${2:-}"; shift 2 ;;
    --interval) INTERVAL_SECONDS="${2:-}"; shift 2 ;;
    --done-after) DONE_AFTER_SECONDS="${2:-}"; shift 2 ;;
    --idle-after) IDLE_AFTER_SECONDS="${2:-}"; shift 2 ;;
    --heartbeat) HEARTBEAT_SECONDS="${2:-}"; shift 2 ;;
    --once) ONCE=1; shift ;;
    --background) BACKGROUND=1; shift ;;
    --stop) STOP=1; shift ;;
    --status) STATUS=1; shift ;;
    --pid-file) PID_FILE="${2:-}"; shift 2 ;;
    --log-file) LOG_FILE="${2:-}"; shift 2 ;;
    -v|--verbose) VERBOSE=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

MODE="$(printf '%s' "$MODE" | tr '[:upper:]' '[:lower:]')"
if [ "$MODE" != "device" ] && [ "$MODE" != "auto" ] && [ "$MODE" != "codex" ] && [ "$MODE" != "claude" ]; then
  echo "--mode must be device, auto, codex, or claude" >&2
  exit 2
fi

log() {
  if [ "$VERBOSE" = "1" ]; then
    printf '[agent-watch] %s\n' "$*" >&2
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
    printf 'agent-watch already running: pid %s\n' "$existing_pid"
    return 0
  fi
  if [ "$ONCE" = "1" ]; then
    echo "--background cannot be used with --once" >&2
    exit 2
  fi

  mkdir -p "$(dirname "$PID_FILE")" "$(dirname "$LOG_FILE")"
  child_args=(
    --codex-sessions-dir "$CODEX_SESSIONS_DIR"
    --claude-sessions-dir "$CLAUDE_SESSIONS_DIR"
    --device-url "$DEVICE_URL"
    --stage-script "$STAGE_SCRIPT"
    --mode "$MODE"
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
  printf 'agent-watch started: pid %s, log %s\n' "$!" "$LOG_FILE"
}

stop_background() {
  local pid
  pid="$(read_pid_file || true)"
  if ! pid_is_running "$pid"; then
    rm -f "$PID_FILE"
    echo "agent-watch is not running"
    return 0
  fi

  kill "$pid"
  echo "agent-watch stopping"
}

show_status() {
  local pid
  pid="$(read_pid_file || true)"
  if pid_is_running "$pid"; then
    printf 'agent-watch running: pid %s\n' "$pid"
  else
    printf 'agent-watch not running\n'
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
  local dir="$1"
  local best=""
  local best_mtime=0
  local file
  local mtime

  if [ ! -d "$dir" ]; then
    return 1
  fi

  while IFS= read -r -d '' file; do
    mtime="$(mtime_epoch "$file" 2>/dev/null || printf '0')"
    if [ "$mtime" -gt "$best_mtime" ]; then
      best="$file"
      best_mtime="$mtime"
    fi
  done < <(find "$dir" -type f -name '*.jsonl' -print0 2>/dev/null)

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

state_rank() {
  case "$1" in
    BLOCK) printf '6' ;;
    TEST) printf '5' ;;
    CODE) printf '4' ;;
    PLAN) printf '3' ;;
    DONE) printf '2' ;;
    IDLE) printf '1' ;;
    *) printf '0' ;;
  esac
}

client_status() {
  local slug="$1"
  local dir="$2"
  local now="$3"
  local latest mtime age has_lifecycle is_complete state message

  latest="$(latest_session_file "$dir" || true)"
  if [ -z "$latest" ]; then
    printf '%s|OFFLINE|%s-offline|0\n' "$slug" "$slug"
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

  if [ "$has_lifecycle" = "1" ] && [ "$is_complete" = "1" ] && [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
    state="DONE"
    message="turn-complete"
  elif [ "$age" -le "$DONE_AFTER_SECONDS" ]; then
    state="PLAN"
    message="$slug-session"
  elif [ "$has_lifecycle" = "0" ] && [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
    state="DONE"
    message="turn-complete"
  elif [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
    state="CODE"
    message="active"
  else
    state="OFFLINE"
    message="$slug-offline"
  fi

  printf '%s|%s|%s|%s\n' "$slug" "$state" "$message" "$mtime"
}

LAST_KNOWN_MODE=""

read_device_mode() {
  local body mode
  body="$(curl --noproxy "*" -fsS --max-time 2 "$DEVICE_URL/state" 2>/dev/null || true)"
  mode="$(printf '%s' "$body" | sed -n 's/.*"agentMode":"\([^"]*\)".*/\1/p' | tr '[:upper:]' '[:lower:]')"
  case "$mode" in
    auto|codex|claude)
      LAST_KNOWN_MODE="$mode"
      printf '%s\n' "$mode"
      return 0
      ;;
  esac

  if [ -n "$LAST_KNOWN_MODE" ]; then
    printf '%s\n' "$LAST_KNOWN_MODE"
  else
    printf 'auto\n'
  fi
}

effective_mode() {
  if [ "$MODE" = "device" ]; then
    read_device_mode
  else
    printf '%s\n' "$MODE"
  fi
}

select_status() {
  local mode="$1"
  local codex="$2"
  local claude="$3"
  local c_source c_state c_msg c_mtime h_source h_state h_msg h_mtime c_rank h_rank
  IFS='|' read -r c_source c_state c_msg c_mtime <<<"$codex"
  IFS='|' read -r h_source h_state h_msg h_mtime <<<"$claude"

  case "$mode" in
    codex)
      if [ "$c_state" = "OFFLINE" ]; then
        printf 'none|OFFLINE|codex-offline\n'
      else
        printf 'codex|%s|%s\n' "$c_state" "$c_msg"
      fi
      return 0
      ;;
    claude)
      if [ "$h_state" = "OFFLINE" ]; then
        printf 'none|OFFLINE|claude-offline\n'
      else
        printf 'claude|%s|%s\n' "$h_state" "$h_msg"
      fi
      return 0
      ;;
  esac

  if [ "$c_state" = "OFFLINE" ] && [ "$h_state" = "OFFLINE" ]; then
    printf 'none|OFFLINE|agents-offline\n'
    return 0
  fi
  if [ "$h_state" = "OFFLINE" ]; then
    printf 'codex|%s|%s\n' "$c_state" "$c_msg"
    return 0
  fi
  if [ "$c_state" = "OFFLINE" ]; then
    printf 'claude|%s|%s\n' "$h_state" "$h_msg"
    return 0
  fi

  c_rank="$(state_rank "$c_state")"
  h_rank="$(state_rank "$h_state")"
  if [ "$c_rank" -gt "$h_rank" ]; then
    printf 'codex|%s|%s\n' "$c_state" "$c_msg"
  elif [ "$h_rank" -gt "$c_rank" ]; then
    printf 'claude|%s|%s\n' "$h_state" "$h_msg"
  elif [ "$c_mtime" -ge "$h_mtime" ]; then
    printf 'codex|%s|%s\n' "$c_state" "$c_msg"
  else
    printf 'claude|%s|%s\n' "$h_state" "$h_msg"
  fi
}

current_source=""
current_state=""
current_message=""
last_heartbeat=0
cleanup_done=0

push_state() {
  local source="$1"
  local state="$2"
  local message="$3"
  log "push $source $state $message"
  "$STAGE_SCRIPT" -DeviceUrl "$DEVICE_URL" -State "$state" -Message "$message" -Source "$source" >/dev/null 2>&1 || true
  current_source="$source"
  current_state="$state"
  current_message="$message"
  last_heartbeat="$(date +%s)"
}

cleanup() {
  local pid
  if [ "$cleanup_done" = "1" ]; then
    return 0
  fi
  cleanup_done=1
  if [ "$ONCE" = "0" ]; then
    push_state "none" "OFFLINE" "agents-offline"
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
  local now mode codex claude selected source state message
  now="$(date +%s)"
  mode="$(effective_mode)"
  codex="$(client_status codex "$CODEX_SESSIONS_DIR" "$now")"
  claude="$(client_status claude "$CLAUDE_SESSIONS_DIR" "$now")"
  selected="$(select_status "$mode" "$codex" "$claude")"
  IFS='|' read -r source state message <<<"$selected"

  if [ "$source" != "$current_source" ] || [ "$state" != "$current_state" ] || [ "$message" != "$current_message" ]; then
    push_state "$source" "$state" "$message"
    return 0
  fi

  if [ "$((now - last_heartbeat))" -ge "$HEARTBEAT_SECONDS" ]; then
    push_state "$source" "$state" "$message"
  fi
}

while true; do
  poll_once
  if [ "$ONCE" = "1" ]; then
    break
  fi
  sleep "$INTERVAL_SECONDS"
done
