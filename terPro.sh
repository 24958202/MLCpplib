#!/usr/bin/env bash
# kill_processes.sh
# Repeatedly checks for target processes and terminates them.
# Works on Linux and macOS.
# Poll interval default: 200 ms (configurable via POLL_MS env var or -i flag).
#
# Usage:
#   ./kill_processes.sh -p "procname1,procname2,..." [-i 200] [-f]
#   or set env vars:
#     TARGET_PROCS="proc1,proc2" POLL_MS=200 ./kill_processes.sh
#
# Notes:
# - Matches by process name (command name as seen in ps/pgrep).
# - Use -f to match full command line (pgrep -f).
# - Excludes the script’s own PID.

set -euo pipefail

POLL_MS="${POLL_MS:-200}"
FULLCMD_MATCH=false
TARGET_PROCS="${TARGET_PROCS:-}"

usage() {
  echo "Usage: $0 -p \"name1,name2,...\" [-i <ms>] [-f]"
  echo "  -p   Comma-separated process names to target (required unless TARGET_PROCS env set)"
  echo "  -i   Poll interval in milliseconds (default: ${POLL_MS})"
  echo "  -f   Match full command line (pgrep -f); default matches command name only"
  exit 1
}

# Parse args
while getopts ":p:i:f" opt; do
  case "$opt" in
    p) TARGET_PROCS="$OPTARG" ;;
    i) POLL_MS="$OPTARG" ;;
    f) FULLCMD_MATCH=true ;;
    *) usage ;;
  esac
done

if [[ -z "${TARGET_PROCS}" ]]; then
  echo "Error: target process list is required."
  usage
fi

# Validate POLL_MS is a positive integer
if ! [[ "$POLL_MS" =~ ^[0-9]+$ ]] || [[ "$POLL_MS" -le 0 ]]; then
  echo "Error: poll interval must be a positive integer (milliseconds)."
  exit 2
fi

# Determine sleep command that supports sub-second timing
SLEEP_CMD="sleep"
SLEEP_ARG="$(awk -v ms="$POLL_MS" 'BEGIN{printf "%.3f", ms/1000}')"

# Ensure pgrep/pkill exist
if ! command -v pgrep >/dev/null 2>&1 || ! command -v pkill >/dev/null 2>&1; then
  echo "Error: pgrep/pkill required but not found. Install procps (Linux) or use macOS default tools."
  exit 3
fi

# Convert comma/space-separated list to array
IFS=', ' read -r -a PROC_LIST <<< "$TARGET_PROCS"

# Build pgrep options
PGREP_OPTS=()
PKILL_OPTS=()
if $FULLCMD_MATCH; then
  PGREP_OPTS+=("-f")
  PKILL_OPTS+=("-f")
fi

SELF_PID="$"

echo "Monitoring processes every ${POLL_MS} ms..."
echo "Targets: ${PROC_LIST[*]}"
$FULLCMD_MATCH && echo "Matching mode: full command line" || echo "Matching mode: command name"

# Helper: get PIDs for a single name, excluding self
get_pids_for_name() {
  local name="$1"
  # -x ensures exact match of the name when not using -f; keep off when -f is set
  local opts=("${PGREP_OPTS[@]}")
  if ! $FULLCMD_MATCH; then
    opts+=("-x")
  fi
  # pgrep returns nonzero if none found; suppress error output
  pgrep "${opts[@]}" -- "$name" 2>/dev/null | awk -v self="$SELF_PID" '$1 != self'
}

terminate_pids() {
  local name="$1"
  local pids=("$@"); pids=("${pids[@]:1}") # drop name
  if [[ "${#pids[@]}" -eq 0 ]]; then
    return 0
  fi
  echo "Found $name: PIDs ${pids[*]} -> sending SIGTERM"
  # Graceful stop
  pkill "${PKILL_OPTS[@]}" -TERM -x -- "$name" 2>/dev/null || true
  # Wait briefly (500 ms) to allow clean exit
  sleep 0.5
  # Re-check remaining PIDs
  local remaining=()
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      remaining+=("$pid")
    fi
  done
  if [[ "${#remaining[@]}" -gt 0 ]]; then
    echo "Forcing SIGKILL for $name: remaining PIDs ${remaining[*]}"
    pkill "${PKILL_OPTS[@]}" -KILL -x -- "$name" 2>/dev/null || true
  fi
}

# Main loop
while true; do
  for name in "${PROC_LIST[@]}"; do
    [[ -z "$name" ]] && continue
    mapfile -t pids < <(get_pids_for_name "$name" || true)
    if [[ "${#pids[@]}" -gt 0 ]]; then
      terminate_pids "$name" "${pids[@]}"
    fi
  done
  $SLEEP_CMD "$SLEEP_ARG"
done
