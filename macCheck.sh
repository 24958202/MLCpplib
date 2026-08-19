#!/bin/bash
#==============================================================================
# macCheck.sh — Comprehensive MacBook Security Audit Script (Upgraded)
#
# Performs a full security audit covering:
#   1. Persistence mechanisms (LaunchAgents, Daemons, BTM, shell profiles)
#   2. Network exposure (listening ports, DNS, AirPlay, remote access)
#   3. Process risk analysis (DYLD, suspicious names, setuid binaries)
#   4. Service integrity verification (SHA-256 hash comparison)
#   5. Security features posture (FileVault, SIP, XProtect, Firewall)
#   6. Allowlist enforcement (compare running processes against allowlist)
#
# Usage:  sudo chmod +x macCheck.sh && sudo ./macCheck.sh
#==============================================================================

set -euo pipefail

# ── Ensure Root Privilege but Retain User Context ─────────────────────────────
if [[ $EUID -ne 0 ]]; then
    echo "❌ Please run this script with sudo: sudo ./macCheck.sh"
    exit 1
fi

# Get the actual user who invoked sudo, and their real home directory
ACTUAL_USER="${SUDO_USER:-$(whoami)}"
USER_HOME=$(dscl . -read /Users/"$ACTUAL_USER" NFSHomeDirectory | awk '{print $2}')

# ── Configuration ─────────────────────────────────────────────────────────────
ALLOWLIST_FILE="$USER_HOME/.config/mac_guard/allowlist_v2.txt"
REPORT_FILE="$USER_HOME/Desktop/audit_report_$(date +%Y%m%d_%H%M%S).txt"

BASELINE_HASHES=(
    "6fac7f05491c15a4feee4fb111641078208693de509a91ed58823ec7e730cc8b"  # party.ape.helper plist
    "5c5b4d03ab099ff88ed43edcf4db9e74b37c7e8c00374ee8fdec9e51e5c1225e"  # party.ape.helper binary
    "c822713448ca59616dc0ffb1a51fbc20191878595cb181abcd7cc607f2b16222"  # com.macguard.agent plist
    "366547d721b5c999ce927fe1b23639b983fd5075f40cf9dfab11267a88edb38e"  # com.macguard.watchdog plist
    "a4b56e8a955d33f8d324fbe33550711c7f995fda874319a8937793662b385926"  # mac_guard.sh
    "817cd06f994d1146bddf53b0616c3166fc1581882b83f5ef5159684f87e4ec3c"  # mac_guard_watchdog.sh
)
SERVICE_FILES=(
    "/Library/LaunchDaemons/party.ape.helper.plist"
    "/Library/PrivilegedHelperTools/party.ape.helper"
    "/tmp/com.macguard.agent.plist"
    "/tmp/com.macguard.watchdog.plist"
    "/tmp/mac_guard.sh"
    "/tmp/mac_guard_watchdog.sh"
)

# Added mac_guard exclusions to prevent false positives
SUSPICIOUS_PATTERN='nc |ncat|socat|cryptominer|xmr|minerd|kworker|payload|shell|reverse|backdoor|keylog|rat |vnc |teamview|anydesk'
IGNORE_PATTERN='workbud|WindowServer|loginwin|mac_guard|macCheck'

ISSUES_FOUND=0

# ── Helper Functions ──────────────────────────────────────────────────────────
log()    { echo "$@"; }
logsep() { echo "════════════════════════════════════════════════════════════════"; }
pass()   { echo "  ✅ $1"; }
warn()   { echo "  ⚠️  $1"; ((ISSUES_FOUND++)); }
fail()   { echo "  ❌ $1"; ((ISSUES_FOUND++)); }
info()   { echo "  ℹ️  $1"; }

run_audit() {
    logsep
    log "  MacBook Security Audit — $(date '+%Y-%m-%d %H:%M:%S')"
    log "  User Context: $ACTUAL_USER  Host: $(hostname)"
    log "  macOS: $(sw_vers -productVersion) ($(sw_vers -buildVersion))"
    logsep
    echo
}

# ── 1. Persistence Mechanisms ─────────────────────────────────────────────────
check_persistence() {
    log "━━━ 1. PERSISTENCE MECHANISMS ━━━"
    echo

    # Filtered LaunchAgents & Daemons (Hiding com.apple noise)
    log "── Non-Apple LaunchAgents ($USER_HOME/Library/LaunchAgents) ──"
    ls -la "$USER_HOME/Library/LaunchAgents/" 2>/dev/null | grep -v "com.apple." || echo "  (none or all Apple native)"
    echo

    log "── Non-Apple System LaunchDaemons (/Library/LaunchDaemons) ──"
    ls -la /Library/LaunchDaemons/ 2>/dev/null | grep -v "com.apple." || echo "  (none or all Apple native)"
    echo

    # Modern macOS Background Task Management
    log "── Background Items (macOS 13+) ──"
    sfltool dumpbtm 2>/dev/null | grep -E "Name:|Developer:" | grep -v "Apple" | head -n 10 || echo "  (none/error)"
    echo

    # Shell profiles (Common malware target)
    log "── Shell Profiles (Check for rogue aliases/exports) ──"
    for profile in .zshrc .zshenv .zprofile .bash_profile .bashrc; do
        if [[ -f "$USER_HOME/$profile" ]]; then
            info "Found ~/$profile (Last modified: $(stat -f "%Sm" "$USER_HOME/$profile"))"
        fi
    done
    echo

    log "── Cron Jobs & Periodic Scripts ──"
    crontab -u "$ACTUAL_USER" -l 2>/dev/null || echo "  (no user cron)"
    ls -la /etc/cron.d/ 2>/dev/null || echo "  (no system cron)"
    echo

    log "── Kernel & System Extensions (non-Apple) ──"
    kextstat 2>/dev/null | grep -v com.apple || echo "  (no 3rd-party kexts)"
    systemextensionsctl list 2>/dev/null | grep -v "0 extensions" || echo "  (none)"
    echo
}

# ── 2. Network Exposure ───────────────────────────────────────────────────────
check_network() {
    log "━━━ 2. NETWORK EXPOSURE ━━━"
    echo

    # Network connections filtered for highest risk
    log "── Established Outbound (non-localhost) ──"
    lsof -i -P -n 2>/dev/null | grep ESTABLISHED | grep -v "127.0.0.1" | head -20 || echo "  (none)"
    echo

    log "── AirPlay Receiver (ports 7000/5000) ──"
    if lsof -i :7000 -P -n 2>/dev/null | grep LISTEN >/dev/null || lsof -i :5000 -P -n 2>/dev/null | grep LISTEN >/dev/null; then
        warn "AirPlay Receiver is LISTENING on all interfaces"
        echo "  Fix: System Settings → General → AirDrop & Handoff → AirPlay Receiver → Off"
    else
        pass "AirPlay Receiver not listening"
    fi
    echo

    log "── SSH / Remote Login ──"
    if pgrep -fl sshd 2>/dev/null | grep -v grep >/dev/null; then
        warn "SSH (Remote Login) is currently active"
    else
        pass "SSH disabled"
    fi

    log "── VNC / Screen Sharing ──"
    if lsof -i :5900 -P -n 2>/dev/null | grep LISTEN >/dev/null || pgrep -fl ARDAgent >/dev/null; then
        warn "VNC or Remote Management is running"
    else
        pass "VNC/Screen sharing disabled"
    fi
    echo
}

# ── 3. Process Risk Analysis ──────────────────────────────────────────────────
check_processes() {
    log "━━━ 3. PROCESS RISK ANALYSIS ━━━"
    echo

    log "── DYLD_INSERT_LIBRARIES Injection ──"
    local dyld
    dyld=$(ps auxww 2>/dev/null | grep -i "DYLD_INSERT" | grep -v grep || true)
    if [[ -n "$dyld" ]]; then
        fail "DYLD injection detected:"
        echo "$dyld"
    else
        pass "No DYLD injection found"
    fi

    log "── Suspicious Process Names ──"
    local suspicious
    suspicious=$(ps axco command 2>/dev/null | grep -iE "$SUSPICIOUS_PATTERN" | grep -vE "$IGNORE_PATTERN" || true)
    if [[ -n "$suspicious" ]]; then
        warn "Suspicious processes found (Review carefully):"
        echo "$suspicious"
    else
        pass "No suspicious process names"
    fi
    echo

    log "── setuid Binaries (non-Apple paths) ──"
    local setuid_bins
    setuid_bins=$(find /usr/local/bin /opt/homebrew/bin "$USER_HOME/bin" -type f -perm -4000 2>/dev/null || true)
    if [[ -n "$setuid_bins" ]]; then
        warn "setuid binaries found in non-Apple paths:"
        echo "$setuid_bins"
    else
        pass "No setuid binaries in non-Apple paths"
    fi

    # mihomo check
    log "── VPN/Proxy Status (mihomo) ──"
    if pgrep -fl mihomo 2>/dev/null | grep -v grep >/dev/null; then
        info "mihomo is running (Known-good user proxy)"
    fi
    if [[ -u /usr/local/bin/mihomo ]] 2>/dev/null; then
        warn "mihomo has setuid bit in /usr/local/bin — should be removed if not required"
    fi
    echo

    log "── Recently Modified Executables (7 days) ──"
    # Actively excluding terPro.sh as requested
    find "$USER_HOME/Downloads" "$USER_HOME/Desktop" -type f -perm +111 -mtime -7 2>/dev/null | grep -v "MLCpplib/terPro.sh" | head -15 || echo "  (none)"
    echo
}

# ── 4. Service Integrity Verification ─────────────────────────────────────────
check_service_integrity() {
    log "━━━ 4. SERVICE INTEGRITY VERIFICATION ━━━"
    echo

    local labels=(
        "party.ape.helper.plist"
        "party.ape.helper binary"
        "com.macguard.agent.plist"
        "com.macguard.watchdog.plist"
        "mac_guard.sh"
        "mac_guard_watchdog.sh"
    )

    for i in "${!SERVICE_FILES[@]}"; do
        local file="${SERVICE_FILES[$i]}"
        local baseline="${BASELINE_HASHES[$i]}"
        local label="${labels[$i]}"

        if [[ ! -f "$file" ]]; then
            warn "$label: FILE MISSING ($file)"
            continue
        fi

        local current_hash
        current_hash=$(shasum -a 256 "$file" 2>/dev/null | awk '{print $1}')

        if [[ "$current_hash" == "$baseline" ]]; then
            pass "$label: Valid"
        else
            fail "$label: HASH MISMATCH! (Got: ${current_hash:0:10}...)"
        fi
    done
    echo
}

# ── 5. Security Features Posture ──────────────────────────────────────────────
check_security_features() {
    log "━━━ 5. SECURITY FEATURES POSTURE ━━━"
    echo

    # FileVault
    if fdesetup status 2>/dev/null | grep -q "On"; then
        pass "FileVault: Enabled"
    else
        warn "FileVault: Disabled or Unknown"
    fi

    # SIP
    if csrutil status 2>/dev/null | grep -q "enabled"; then
        pass "SIP: Enabled"
    else
        fail "SIP: Disabled (Critical Risk)"
    fi

    # Gatekeeper
    if spctl --status 2>/dev/null | grep -q "assessments enabled"; then
        pass "Gatekeeper: Enabled"
    else
        warn "Gatekeeper: Disabled"
    fi

    # Firewall
    if /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate 2>/dev/null | grep -q "enabled"; then
        pass "Firewall: Enabled"
    else
        warn "Firewall: Disabled"
    fi

    # XProtect Version (macOS built-in AV)
    local xp_vers
    xp_vers=$(defaults read /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/XProtect.meta.plist Version 2>/dev/null || echo "Unknown")
    pass "XProtect Signatures: v$xp_vers"
    echo
}

# ── 6. Allowlist Enforcement ──────────────────────────────────────────────────
check_allowlist() {
    log "━━━ 6. ALLOWLIST ENFORCEMENT ━━━"
    echo

    if [[ ! -f "$ALLOWLIST_FILE" ]]; then
        warn "Allowlist file not found: $ALLOWLIST_FILE"
        echo
        return
    fi

    local proc_names
    # Checking processes for the actual user, stripping path to get just the binary name
    proc_names=$(pgrep -u "$ACTUAL_USER" -l 2>/dev/null | awk '{print $2}' | sed 's|.*/||' | sort -u)
    
    local allowed_procs
    allowed_procs=$(sed -n '/\[ALLOWED_PROCESSES\]/,$p' "$ALLOWLIST_FILE" | grep -v '^\[' | grep -v '^#' | grep -v '^$' | sort -u)

    local missing_count=0
    local total_count=0

    while IFS= read -r proc; do
        [[ -z "$proc" ]] && continue
        # Apply ScreenSaver exclusions here programmatically
        if [[ "$proc" =~ "ScreenSaver" ]]; then continue; fi
        
        ((total_count++))
        if ! echo "$allowed_procs" | grep -qx "$proc"; then
            echo "  MISSING: $proc"
            ((missing_count++))
        fi
    done <<< "$proc_names"

    echo
    if [[ $missing_count -eq 0 ]]; then
        pass "All $total_count user GUI processes are on the allowlist"
    else
        info "Total distinct GUI process names: $total_count"
        warn "$missing_count processes not on allowlist (Match rate: $(( (total_count - missing_count) * 100 / (total_count > 0 ? total_count : 1) ))%)"
    fi
    echo
}

# ── 7. Summary ────────────────────────────────────────────────────────────────
print_summary() {
    logsep
    log "  AUDIT SUMMARY"
    logsep
    echo

    if [[ $ISSUES_FOUND -eq 0 ]]; then
        pass "AUDIT CLEAN — No security issues found"
    else
        warn "AUDIT COMPLETE — $ISSUES_FOUND issue(s) found (see above)"
    fi

    echo
    log "  Active Exclusions Applied in Script:"
    log "    - $USER_HOME/Downloads/MLCpplib/terPro.sh (Ignored in recent execs)"
    log "    - mac_guard, mac_guard_watchdog (Ignored in suspicious processes)"
    log "    - ScreenSaverEngine (Ignored in allowlist)"
    echo
    log "  Report saved to: $REPORT_FILE"
    logsep
}

# ── Main ──────────────────────────────────────────────────────────────────────
main() {
    # Tee all output to both terminal and report file
    exec > >(tee "$REPORT_FILE") 2>&1

    run_audit
    check_persistence
    check_network
    check_processes
    check_service_integrity
    check_security_features
    check_allowlist
    print_summary
}

main "$@"