#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Real unisacc ABI, carried library and assembly kernel, not a host-C bridge.
#
# This was one file and one gate job, and it measured 61 s warm -- past the
# 60 s watchdog, killed with rc=142 before any of its asserts ran.  A warm
# trace found the cost was not spread evenly: prep 5 s, `elf.sh` over four
# files 31 s, the assert block 9 s.  The gate therefore runs the three parts
# as three jobs (bindprep / bindelf / bindverify, see tests/gate.sh) and each
# is now well inside the bound.
#
# Called with no argument it still runs all three in sequence, so a direct
# invocation keeps working; the artefacts are shared through $X, so the
# second and third steps are cheap when the first has already run.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd); cd "$R"
[ "$(uname -s)" = Darwin ] || { echo 'blob seed construction requires macOS' >&2; exit 2; }
ARCH=${CORE_ASM_ARCH:-$(uname -m)}
case $ARCH in arm64|x86_64) ;; *) exit 2;; esac

case "${1:-all}" in
    prep)   exec "$R/exec/c/asm/bindprep.sh" ;;
    elf|verify) exec "$R/exec/c/asm/bindverify.sh" ;;
    all)    ;;
    *)      echo 'usage: bindingcheck.sh [prep|elf|verify]  (no argument runs all three)' >&2; exit 2 ;;
esac

CORE_ASM_ARCH=$ARCH "$R/exec/c/asm/bindprep.sh"
CORE_ASM_ARCH=$ARCH "$R/exec/c/asm/bindverify.sh"
