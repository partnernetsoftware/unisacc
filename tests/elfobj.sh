#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# R16-7/R17-1: `-c -b lnx|osx|win/ARCH` writes ELF / Mach-O / COFF objects that GNU ld, lld, ld64 and lld-link link
# into a program printing what the image prints.  The checks, the invariant
# against the image and the (named) skips are in tests/elfobj.py; this wrapper
# only supplies the reference compiler.  Linking needs ld.lld (host) or ld
# (Linux); running the arm64 programs needs the Lima VM named by ELFOBJ_VM
# (default: default) to be up -- it is a skip otherwise, a failure with STRICT=1.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
# The COFF run leg needs the Windows VM up; it is asked for only when
# ELFOBJ_WINVM names it (the release queue does not start VMs; the
# pre-release list in release/RELEASE-PIPELINE.md §10 runs it).
"$_BOUND" 60 python3 tests/elfobj.py "$UA" --link --run-arm64-lima "${ELFOBJ_VM:-default}" ${ELFOBJ_WINVM:+--run-windows "$ELFOBJ_WINVM"}
