#!/bin/bash
# kill_processes.sh
# Repeatedly checks for target processes and terminates them.
# Works on Linux and macOS (including the default Bash 3.2 on macOS).
# Poll interval default: 200 ms (configurable via POLL_MS env var or -i flag).

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

# Initialize arrays early (important with set -u on macOS Bash 3.2)
PGREP_OPTS=()
PKILL_OPTS=()

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

# Ensure pgrep/pkill exist
if ! command -v pgrep >/dev/null 2>&1 || ! command -v pkill >/dev/null 2>&1; then
  echo "Error: pgrep/pkill required but not found. Install procps (Linux) or use macOS default tools."
  exit 3
fi

# Convert comma/space-separated list to array
IFS=', ' read -r -a PROC_LIST <<< "$TARGET_PROCS"

# Build pgrep/pkill options
if $FULLCMD_MATCH; then
  PGREP_OPTS+=("-f")
  PKILL_OPTS+=("-f")
fi

# Correctly set self PID
SELF_PID="$"

echo "Monitoring processes every ${POLL_MS} ms..."
echo "Targets: ${PROC_LIST[*]}"
if $FULLCMD_MATCH; then
  echo "Matching mode: full command line"
else
  echo "Matching mode: command name"
fi

# Helper: get PIDs for a single name, excluding self
get_pids_for_name() {
  local name="$1"
  # Use nounset-safe array expansion pattern
  local opts=()
  # Append PGREP_OPTS if any
  eval 'opts+=(${PGREP_OPTS[@]+"${PGREP_OPTS[@]}"})'
  # -x ensures exact match when not using -f
  if ! $FULLCMD_MATCH; then
    opts+=("-x")
  fi
  # pgrep returns nonzero if none found; suppress error output
  pgrep "${opts[@]}" -- "$name" 2>/dev/null | awk -v self="$SELF_PID" '$1 != self'
}

terminate_pids() {
  local name="$1"
  shift
  local pids=("$@")
  if [[ "${#pids[@]}" -eq 0 ]]; then
    return 0
  fi
  echo "Found $name: PIDs ${pids[*]} -> sending SIGTERM"

  # Graceful stop
  if $FULLCMD_MATCH; then
    pkill ${PKILL_OPTS[@]+"${PKILL_OPTS[@]}"} -TERM -- "$name" 2>/dev/null || true
  else
    pkill ${PKILL_OPTS[@]+"${PKILL_OPTS[@]}"} -TERM -x -- "$name" 2>/dev/null || true
  fi

  # Wait briefly (500 ms) to allow clean exit
  sleep 0.5

  # Re-check remaining PIDs
  local remaining=()
  local pid
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      remaining+=("$pid")
    fi
  done

  if [[ "${#remaining[@]}" -gt 0 ]]; then
    echo "Forcing SIGKILL for $name: remaining PIDs ${remaining[*]}"
    if $FULLCMD_MATCH; then
      pkill ${PKILL_OPTS[@]+"${PKILL_OPTS[@]}"} -KILL -- "$name" 2>/dev/null || true
    else
      pkill ${PKILL_OPTS[@]+"${PKILL_OPTS[@]}"} -KILL -x -- "$name" 2>/dev/null || true
    fi
  fi
}

# Main loop
while true; do
  for name in "${PROC_LIST[@]}"; do
    [[ -z "$name" ]] && continue
    # Bash 3.2-compatible replacement for mapfile
    pids=()
    while IFS= read -r pid; do
      [[ -n "$pid" ]] && pids+=("$pid")
    done < <(get_pids_for_name "$name" || true)

    if [[ "${#pids[@]}" -gt 0 ]]; then
      terminate_pids "$name" "${pids[@]}"
    fi
  done

  # Sleep in seconds with millisecond precision
  SLEEP_SEC="$(awk -v ms="$POLL_MS" 'BEGIN{printf "%.3f", ms/1000}')"
  sleep "$SLEEP_SEC"
done