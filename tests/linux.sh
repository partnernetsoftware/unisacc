#!/bin/bash
# The whole suite, on Linux, in a local VM.  [A-32]
#
# This is what the GitHub Linux job used to do.  That workflow is parked now
# (`.github/workflows-disabled/`): the runners are metered, the macOS ones
# bill at ten times the Linux rate, and a push-triggered matrix spends the
# budget on work this desk does for free.  The point was never "a second
# machine" -- it is that the ELF we emit has to be a real program under a real
# kernel, and a VM on this desk is as real as a runner in a datacentre.  It
# also catches what macOS cannot: `difftest` compares against the SYSTEM
# compiler, and glibc and BSD libc disagree about values C leaves unspecified.
#
#   ./tests/linux.sh              the whole suite
#   ./tests/linux.sh difftest     one suite
#
# The VM mounts this repo READ-ONLY, and the suites write (build_ref.sh emits
# unisacc.c beside the sources), so the tree is copied into the guest first --
# minus .git and corpus/, with corpus/ symlinked back to the mount because the
# suites only ever read it.
#
# Why Lima and not UTM: the UTM Linux machines have no QEMU guest agent, so
# `utmctl exec` and `utmctl file` fail on them.  UTM drives Windows, where the
# agent IS installed -- see tests/crossnative.sh.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
VM=${LIMA_VM:-default}
what=${1:-all}

command -v limactl >/dev/null || { echo "linux: no limactl -- skipped"; exit 0; }
case "$(limactl list "$VM" --format '{{.Status}}' 2>/dev/null)" in
    Running) ;;
    *) echo "linux: VM '$VM' is not running -- skipped"; exit 0;;
esac

# The tree is packed HERE and piped in, rather than unpacked from a mount
# inside the guest: only the `default` VM mounts this repository, and
# assuming the mount made `LIMA_VM=minicon-lnx-x86_64` fail with a tar error
# that said nothing about mounts.  corpus/ travels too (4 MB) -- it used to
# be symlinked back to the mount, which for the same reason was not there.
# A guest of a DIFFERENT architecture is emulated, and everything in it
# runs about ten times slower: the per-suite watchdog and the per-probe
# alarms have to be told, or seven suites "fail" by timing out while they
# were working correctly.  The caller's own settings win.
guest_arch=$(limactl list "$VM" --format '{{.Arch}}' 2>/dev/null)
host_arch=$(uname -m); [ "$host_arch" = "arm64" ] && host_arch=aarch64
slow=1
[ -n "$guest_arch" ] && [ "$guest_arch" != "$host_arch" ] && slow=10
LIMIT=${SUITE_LIMIT:-$((900 * slow))}
TRY=${TRY_ALARM:-$((10 * slow))}
[ "$slow" -gt 1 ] && echo "linux: $VM is $guest_arch on $host_arch -- emulated;" \
    "SUITE_LIMIT=$LIMIT TRY_ALARM=$TRY"
# 0.0.22: a full run on an emulated guest is not evidence -- the reference build alone needs
# 76 s there against fixed 45/30 s watchdogs (bound.py caps at 60), and 0.0.21 got ~95 timeouts.
# x86_64 product evidence is the release-check real runner; run targeted suites here instead.
if [ "$slow" -gt 1 ] && [ "$what" = all ] && [ "${ALLOW_EMULATED_ALL:-0}" != 1 ]; then
    echo "linux: refusing 'all' on emulated $VM (release/RELEASE-PIPELINE.md section 4); run a named suite, or ALLOW_EMULATED_ALL=1"; exit 2
fi

# R18-11 ⑤: the guest's own limits are not compiler failures.  On the 4 GiB
# `default` VM the Python compile of the flat compiler source is OOM-killed
# (bigclosure*, ape-prepare*, ape), and the guest has no clang for `warn`.
# They are reported by name as guest-known after the run (STRICT=1 keeps
# them red); every other red still fails.
GUEST_MEM_KB=$(limactl shell "$VM" -- awk '/MemTotal/{print $2}' /proc/meminfo 2>/dev/null | tr -d '\r')
GUEST_CLANG=$(limactl shell "$VM" -- sh -c 'command -v clang >/dev/null && echo yes || echo no' 2>/dev/null | tr -d '\r')
# The guest runs FETCH=0, so the corpus travels in the tarball.  A checkout
# without it (git worktree: /corpus/ is ignored) made diag and warn red with
# "0 diagnosed" / "read nothing from cc" in 0.0.23 -- refuse up front instead.
[ -d "$R/corpus/c-testsuite/tests/single-exec" ] || { echo "linux: $R/corpus is missing (copy it from the main checkout or run tests/corpus.sh with FETCH=1)"; exit 2; }
LOGF=$(mktemp)
tar -C "$R" -cf - --exclude=.git . | limactl shell "$VM" -- bash -lc "
    set -u
    command -v cc >/dev/null || { echo 'linux: no cc in the VM'; exit 1; }
    W=\$HOME/unisa-linux
    rm -rf \$W && mkdir -p \$W
    tar -C \$W -xf -
    cd \$W
    export FETCH=0
    # Guest /tmp is a RAM tmpfs (~4 GB); all.sh's preserved logs and the bigclosure/ape images
    # filled it mid-run in 0.0.23 (27 false reds).  Work on the guest disk instead.
    export TMPDIR=\$HOME/unisa-tmp; rm -rf \$TMPDIR; mkdir -p \$TMPDIR
    export SUITE_LIMIT='$LIMIT' TRY_ALARM='$TRY' JOBS='${JOBS:-4}'
    # Some suites take the probe list; run without it they test NOTHING --
    # closure printed 'identical 0 differ 0' and ccrun reported 90 lost
    # probes, which is its ratchet correctly describing a run of zero.
    # (No backticks in here: this whole command is inside double quotes on
    # the HOST, so a backtick runs there.)
    P=''
    case '$what' in
        native|crossnative|fat|ccrun|selfhost|closure|stages)
            P='examples/*.c tests/c/*.c';;
    esac
    if [ '$what' = all ]; then ./tests/all.sh
    elif [ -n \"\$P\" ]; then sh -c \"./tests/$what.sh \$P\"
    else ./tests/$what.sh; fi
" 2>&1 | tee "$LOGF"
rc=${PIPESTATUS[1]}
if [ "$rc" -ne 0 ] && [ "${STRICT:-0}" != 1 ]; then
    reds=$(grep -E '^finished [^ ]+ rc=[1-9]' "$LOGF" | awk '{print $2}')
    other=""; known=""
    for n in $reds; do
        case "$n" in
            bigclosure*|ape-prepare*|ape) [ "${GUEST_MEM_KB:-0}" -lt 6000000 ] && { known="$known $n"; continue; };;
            warn) [ "$GUEST_CLANG" = no ] && { known="$known $n"; continue; };;
        esac
        other="$other $n"
    done
    if [ -n "$reds" ] && [ -z "$other" ]; then
        echo "linux: only guest-known reds (guest RAM ${GUEST_MEM_KB:-?} kB, clang $GUEST_CLANG):$known"
        rm -f "$LOGF"; exit 0
    fi
    [ -n "$known" ] && echo "linux: guest-known reds:$known"
    [ -n "$other" ] && echo "linux: compiler reds:$other"
fi
rm -f "$LOGF"
exit "$rc"
