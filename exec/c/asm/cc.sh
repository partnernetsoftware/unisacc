#!/bin/sh
# Host test/build adapter for the assembly inference/arithmetic plus remaining C actions.
# This is not a runtime fallback and is not used by the shipped compiler.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd)
arch=${CORE_ASM_ARCH:-$(uname -m)}
case $arch in aarch64) arch=arm64;; arm64|x86_64) ;; *) echo 'unsupported assembly host ISA' >&2; exit 2;; esac
if [ "$(uname -s)" = Darwin ]; then set -- -arch "$arch" "$@"; fi
exec cc "$@" -DUNISA_CORE_EXTERNAL -DUNISA_CORE_ASM_TRANSITION -DUNISA_CORE_ASM_ALU \
    "$R/exec/c/core.c" "$R/exec/c/asm/transition_$arch.S" "$R/exec/c/asm/arith_$arch.S" "$R/exec/c/asm/layoutcheck.c"
