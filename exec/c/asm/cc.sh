#!/bin/sh
# Host test/build adapter for the complete assembly execution kernel.
# This is not a runtime fallback and is not used by the shipped compiler.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd)
arch=${CORE_ASM_ARCH:-$(uname -m)}
case $arch in aarch64) arch=arm64;; arm64|x86_64) ;; *) echo 'unsupported assembly host ISA' >&2; exit 2;; esac
if [ "$(uname -s)" = Darwin ]; then set -- -arch "$arch" "$@"; fi
# Both ISA action engines require the complete assembly primitive set.
exec cc -DUNISA_CORE_ASM_RUN -DUNISA_CORE_ASM_ACTION "$R/exec/c/asm/action_$arch.S" "$@" -DUNISA_CORE_EXTERNAL -DUNISA_CORE_ASM_TRANSITION -DUNISA_CORE_ASM_ALU -DUNISA_CORE_ASM_BUFFER -DUNISA_CORE_ASM_MEMORY -DUNISA_CORE_ASM_INTERN -DUNISA_CORE_ASM_BYTES -DUNISA_CORE_ASM_FORMAT -DUNISA_CORE_ASM_STACK \
    "$R/exec/c/asm/run_$arch.S" "$R/exec/c/asm/transition_$arch.S" "$R/exec/c/asm/arith_$arch.S" "$R/exec/c/asm/buffer_$arch.S" "$R/exec/c/asm/memory_$arch.S" "$R/exec/c/asm/intern_$arch.S" "$R/exec/c/asm/bytes_$arch.S" "$R/exec/c/asm/format_$arch.S" "$R/exec/c/asm/stack_$arch.S" "$R/exec/c/asm/layoutcheck.c"
