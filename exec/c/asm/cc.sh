#!/bin/sh
# Host test/build adapter for the assembly primitives plus remaining C actions.
# This is not a runtime fallback and is not used by the shipped compiler.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd)
arch=${CORE_ASM_ARCH:-$(uname -m)}
case $arch in aarch64) arch=arm64;; arm64|x86_64) ;; *) echo 'unsupported assembly host ISA' >&2; exit 2;; esac
if [ "$(uname -s)" = Darwin ]; then set -- -arch "$arch" "$@"; fi
# Action dispatch is migrated on arm64; x86-64 still selects C explicitly.
ACTION_FLAGS=; ACTION_SOURCE=
if [ "$arch" = arm64 ]; then ACTION_FLAGS=-DUNISA_CORE_ASM_ACTION; ACTION_SOURCE="$R/exec/c/asm/action_arm64.S"; fi
exec cc $ACTION_FLAGS $ACTION_SOURCE "$@" -DUNISA_CORE_EXTERNAL -DUNISA_CORE_ASM_TRANSITION -DUNISA_CORE_ASM_ALU -DUNISA_CORE_ASM_BUFFER -DUNISA_CORE_ASM_MEMORY -DUNISA_CORE_ASM_INTERN -DUNISA_CORE_ASM_BYTES -DUNISA_CORE_ASM_FORMAT -DUNISA_CORE_ASM_STACK \
    "$R/exec/c/core.c" "$R/exec/c/asm/transition_$arch.S" "$R/exec/c/asm/arith_$arch.S" "$R/exec/c/asm/buffer_$arch.S" "$R/exec/c/asm/memory_$arch.S" "$R/exec/c/asm/intern_$arch.S" "$R/exec/c/asm/bytes_$arch.S" "$R/exec/c/asm/format_$arch.S" "$R/exec/c/asm/stack_$arch.S" "$R/exec/c/asm/layoutcheck.c"
