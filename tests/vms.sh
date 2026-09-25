#!/bin/bash
# The release gate's machines, up and down.  [S-15 F1]
#
#   tests/vms.sh up      start what crossnative needs and is not running,
#                        and wait until each answers
#   tests/vms.sh down    stop ONLY what `up` started -- a VM that was already
#                        running when the gate began is left as it was
#
# The gate refuses skips (STRICT=1), so without this a release meant starting
# three VMs by hand and remembering to stop them; the Windows one is
# expensive on CPU and was once left running.  Every command here is bounded
# (AGENTS.md: 60 s a run); booting takes longer than that, so the wait is a
# loop of bounded polls.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
STATE=${VMS_STATE:-${TMPDIR:-/tmp}/unisacc-vms.started}
LIMA_VMS="${VM_X86:-minicon-lnx-x86_64} ${VM_ARM:-default}"
UTM=/Applications/UTM.app/Contents/MacOS/utmctl
WINVM=${WINVM:-minicon-win-arm-64}
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }   # b SECONDS cmd...

lima_status() { limactl list --format '{{.Name}} {{.Status}}' 2>/dev/null | awk -v n="$1" '$1==n{print $2}'; }

up() {
    : > "$STATE"
    if command -v limactl >/dev/null; then
        for vm in $LIMA_VMS; do
            case "$(lima_status "$vm")" in
            Running) echo "  vm $vm already running";;
            "") echo "  vm $vm does not exist";;
            *)  echo "  vm $vm starting"
                # limactl start waits for the guest itself; each attempt is bounded
                b 55 limactl start "$vm" >/dev/null 2>&1
                i=0; while [ "$(lima_status "$vm")" != Running ] && [ $i -lt 24 ]; do
                    sleep 5; i=$((i+1)); done
                if [ "$(lima_status "$vm")" = Running ]; then
                    echo "lima $vm" >> "$STATE"; echo "  vm $vm up"
                else echo "  vm $vm did NOT come up"; fi;;
            esac
        done
    fi
    if [ -x "$UTM" ]; then
        if "$UTM" status "$WINVM" 2>/dev/null | grep -q started; then
            echo "  vm $WINVM already running"
        else
            echo "  vm $WINVM starting"
            b 55 "$UTM" start "$WINVM" >/dev/null 2>&1
            echo "utm $WINVM" >> "$STATE"
            # up means the guest agent answers, not that the VM is powered
            i=0
            while [ $i -lt 36 ]; do
                b 10 "$UTM" exec "$WINVM" --hide --cmd cmd.exe -- /c echo up \
                    >/dev/null 2>&1 && break
                sleep 5; i=$((i+1))
            done
            [ $i -lt 36 ] && echo "  vm $WINVM up" || echo "  vm $WINVM did NOT answer"
        fi
    fi
}

down() {
    [ -f "$STATE" ] || { echo "  vms: nothing was started here"; return 0; }
    while read -r kind vm; do
        case "$kind" in
        lima) b 55 limactl stop "$vm" >/dev/null 2>&1; echo "  vm $vm stopped ($(lima_status "$vm"))";;
        utm)  b 55 "$UTM" stop "$WINVM" >/dev/null 2>&1
              echo "  vm $vm stopped ($("$UTM" status "$vm" 2>/dev/null))";;
        esac
    done < "$STATE"
    rm -f "$STATE"
}

case "${1:-}" in
up) up;;
down) down;;
*) echo "usage: tests/vms.sh up|down"; exit 2;;
esac
