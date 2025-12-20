#!/bin/bash
# Start all 3 apps in the background

# Ensure XDG_RUNTIME_DIR is set to a per-user runtime dir
if [ -z "${XDG_RUNTIME_DIR}" ]; then
    export XDG_RUNTIME_DIR="${HOME}/.xdg-runtime"
    if [ ! -d "${XDG_RUNTIME_DIR}" ]; then
        mkdir -p "${XDG_RUNTIME_DIR}"
        chmod 0700 "${XDG_RUNTIME_DIR}"
    fi
fi

/home/ronnieji/m_ufw_iptables &
/home/ronnieji/faceDetectVideo &
/home/ronnieji/webCamMain &

# Instead of plain `wait`, loop and ignore warnings about stopped jobs
while true; do
    # If there are no background jobs, break out of the loop
    jobs >/dev/null 2>&1 || break
    # Wait for any job; ignore errors/warnings (e.g., on stopped jobs)
    wait -n 2>/dev/null || true
done
