#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
SESSIONS_DIR="${SESSIONS_DIR:-$CODEX_HOME/sessions}"
DEVICE_URL="${DEVICE_URL:-http://192.168.4.1}"
STAGE_SCRIPT="${STAGE_SCRIPT:-$SCRIPT_DIR/codex-stage.sh}"
INTERVAL_SECONDS="${INTERVAL_SECONDS:-2}"
DONE_AFTER_SECONDS="${DONE_AFTER_SECONDS:-20}"
IDLE_AFTER_SECONDS="${IDLE_AFTER_SECONDS:-300}"
HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"
ONCE=0
VERBOSE="${VERBOSE:-0}"

usage() {
  cat <<'EOF'
Usage:
  tools/codex-watch.sh [options]

Options:
  --sessions-dir DIR     Codex sessions directory. Default: $CODEX_HOME/sessions
  --device-url URL       Clawd Mochi URL. Default: http://192.168.4.1
  --stage-script FILE    Status push script. Default: tools/codex-stage.sh
  --interval SECONDS     Poll interval. Default: 2
  --done-after SECONDS   Mark DONE after no session writes. Default: 20
  --idle-after SECONDS   Mark IDLE after longer inactivity. Default: 300
  --heartbeat SECONDS    Re-send current state interval. Default: 60
  --once                 Run one poll iteration, useful for testing.
  -v, --verbose          Print watcher decisions.
  -h, --help             Show this help.

Environment:
  CODEX_HOME, SESSIONS_DIR, DEVICE_URL, STAGE_SCRIPT, INTERVAL_SECONDS,
  DONE_AFTER_SECONDS, IDLE_AFTER_SECONDS, HEARTBEAT_SECONDS, VERBOSE
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

last_file=""
last_mtime=""
last_activity=0
current_state=""
current_message=""
last_heartbeat=0
done_sent=0
idle_sent=0

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
  if [ "$ONCE" = "0" ]; then
    push_state "OFFLINE" "codex-offline"
  fi
}

trap cleanup EXIT INT TERM

poll_once() {
  local now latest mtime age
  now="$(date +%s)"
  latest="$(latest_session_file || true)"

  if [ -z "$latest" ]; then
    last_file=""
    last_mtime=""
    last_activity=0
    done_sent=0
    idle_sent=0
    if [ "$current_state" != "OFFLINE" ]; then
      push_state "OFFLINE" "codex-offline"
    fi
    return 0
  fi

  mtime="$(mtime_epoch "$latest")"
  age=$((now - mtime))

  if [ "$latest" != "$last_file" ]; then
    last_file="$latest"
    last_mtime="$mtime"
    last_activity="$mtime"
    done_sent=0
    idle_sent=0

    if [ "$age" -le "$DONE_AFTER_SECONDS" ]; then
      push_state "PLAN" "codex-session"
    elif [ "$age" -lt "$IDLE_AFTER_SECONDS" ]; then
      push_state "DONE" "turn-complete"
      done_sent=1
    else
      push_state "OFFLINE" "codex-offline"
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
    if [ "$current_state" != "CODE" ]; then
      push_state "CODE" "active"
    fi
    return 0
  fi

  age=$((now - last_activity))
  if [ "$current_state" != "BLOCK" ] && [ "$age" -ge "$IDLE_AFTER_SECONDS" ] && [ "$idle_sent" = "0" ]; then
    push_state "IDLE" "codex-ready"
    idle_sent=1
    return 0
  fi

  if [ "$current_state" != "BLOCK" ] && [ "$age" -ge "$DONE_AFTER_SECONDS" ] && [ "$done_sent" = "0" ]; then
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
