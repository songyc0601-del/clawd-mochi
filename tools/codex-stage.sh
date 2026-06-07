#!/usr/bin/env bash
set -euo pipefail

STATE="${STATE:-STATE}"
MSG="${MESSAGE:-}"
SOURCE="${SOURCE:-}"
DEVICE_URL="${DEVICE_URL:-http://192.168.4.1}"

usage() {
  cat <<'EOF'
Usage:
  tools/codex-stage.sh [-State STATE] [-Message MESSAGE] [-Source SOURCE] [-DeviceUrl URL]
  tools/codex-stage.sh STATE [MESSAGE]

States: OFFLINE, IDLE, PLAN, CODE, TEST, DONE, BLOCK, STATE
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    -State|--state)
      STATE="${2:-}"
      shift 2
      ;;
    -Message|--message|-Msg|--msg)
      MSG="${2:-}"
      shift 2
      ;;
    -Source|--source)
      SOURCE="${2:-}"
      shift 2
      ;;
    -DeviceUrl|--device-url|--url)
      DEVICE_URL="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      STATE="$1"
      shift
      if [ "$#" -gt 0 ]; then
        MSG="$*"
        break
      fi
      ;;
  esac
done

STATE="$(printf "%s" "$STATE" | tr '[:lower:]' '[:upper:]')"

clean_msg="$(printf "%s" "$MSG" | tr -cd '\11\12\15\40-\176' | sed 's/^ *//;s/ *$//')"

if [ "$STATE" = "STATE" ]; then
  curl --noproxy "*" -fsS --max-time 2 "$DEVICE_URL/state"
else
  args=(--noproxy "*" -fsS --max-time 2 --get "$DEVICE_URL/progress" --data-urlencode "state=$STATE" --data-urlencode "msg=$clean_msg")
  if [ -n "$SOURCE" ]; then
    args+=(--data-urlencode "source=$SOURCE")
  fi
  curl "${args[@]}"
fi
