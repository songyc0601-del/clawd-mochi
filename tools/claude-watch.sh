#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

CLAUDE_HOME="${CLAUDE_HOME:-$HOME/.claude}"
export SESSIONS_DIR="${SESSIONS_DIR:-$CLAUDE_HOME/projects}"
export CLIENT_SLUG="${CLIENT_SLUG:-claude}"
export PID_FILE="${PID_FILE:-${XDG_RUNTIME_DIR:-/tmp}/clawd-mochi-claude-watch.pid}"
export LOG_FILE="${LOG_FILE:-${XDG_STATE_HOME:-$HOME/.local/state}/clawd-mochi/claude-watch.log}"

exec "$SCRIPT_DIR/codex-watch.sh" "$@"
