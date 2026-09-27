#!/bin/sh
# Development binding: compile the driver with unisacc's actual internal ABI.
# UNISA_KERNEL selects the separately assembled blob at runtime, not a fallback.
set -eu
case $(uname -s) in Darwin) os=osx;; Linux) os=lnx;; *) echo 'unsupported seed host' >&2; exit 2;; esac
arch=${CORE_ASM_ARCH:-$(uname -m)}
case $arch in aarch64) arch=arm64;; arm64|x86_64) ;; *) exit 2;; esac
exec "${UA:-/tmp/ua_ref}" -b "$os/$arch" -DUNISA_CORE_BLOB "$@"
