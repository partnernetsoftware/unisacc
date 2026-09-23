#!/bin/sh
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
tar -C "$R" -cf - --exclude=.git . | limactl shell "$VM" -- bash -lc "
    set -u
    command -v cc >/dev/null || { echo 'linux: no cc in the VM'; exit 1; }
    W=\$HOME/unisa-linux
    rm -rf \$W && mkdir -p \$W
    tar -C \$W -xf -
    cd \$W
    export FETCH=0
    if [ '$what' = all ]; then ./tests/all.sh; else ./tests/$what.sh; fi
"
