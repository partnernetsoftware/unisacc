#!/bin/sh
# Native API controls use the existing stdlib JSON/SHA reference through its C API.
# Matching development flags come from the interpreter installation's shell config.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"
base=${CXBIN_CACHE:-${TMPDIR:-/tmp}/unisacc-cxbin-$(id -u)}
case "$base" in /*) ;; *) echo 'gatecanoncheck: absolute cache required' >&2; exit 2;; esac
case "$base" in /|/tmp|/home|"${HOME:-/home}") echo 'gatecanoncheck: unsafe cache' >&2; exit 2;; esac
umask 077
[ -d "$base" ] || mkdir -p "$base"
[ ! -L "$base" ] && [ -O "$base" ] || { echo 'gatecanoncheck: unowned cache' >&2; exit 2; }
build=$(mktemp -d "$base/gatecanon-check.XXXXXXXX")
includes=$(tests/bound 5 python3-config --includes)
ldflags=$(tests/bound 5 python3-config --embed --ldflags)
prefix=$(tests/bound 5 python3-config --prefix)
# Standard config returns compiler argument words, not shell source; never eval.
tests/bound 20 cc -std=c11 -O2 $includes tests/gatecanoncheck.c $ldflags -Wl,-rpath,"$prefix/lib" -o "$build/check"
exec tests/bound 20 "$build/check" "$prefix" "$root"
