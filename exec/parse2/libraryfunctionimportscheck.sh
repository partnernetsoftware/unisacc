#!/bin/sh
# Model-only. Old product is the dynamic-C observer; no library API/fresh claim.
set -eu
mode=${1:-check}
case "$mode" in
 --prepare|--prepare-gen|--prepare-c-json|--prepare-p-json|--prepare-tbl|--prepare-net|--prepare-runner|--require-cached|check) ;;
 *) echo 'usage: libraryfunctionimportscheck.sh --prepare-{gen,c-json,p-json,tbl,net,runner} | --require-cached' >&2;exit 2;;
esac
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
b(){ "$R/tests/bound" 20 "$@"; }
# The historical key is retained, scoped to one frozen host/environment. It is
# not a complete cross-host compiler/header/environment identity. No verdict is cached.
key(){ { rg --files exec unisa seed include weights | LC_ALL=C sort | rg '\.(tsv|py|c|h|sh|inc|json)$' | xargs sha256sum; cc --version; python3 --version; printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1; }
K=$(key);D=${TMPDIR:-/tmp}/unisacc-libraryfunctionimports-$K
T=;LOCKED=0
cleanup(){
 if [ -n "$T" ];then sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T" || :;fi
 if [ "$LOCKED" = 1 ];then rmdir "$D/lock" || :;fi
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
need_prepare(){ echo "libraryfunctionimports: NEED_PREPARE $1 (missing, stale or corrupt seal/artifact)" >&2;return 75; }
# Exact static artifact lists prevent a shortened/empty manifest from passing.
# Each seal also hashes its direct predecessor seals; valid() verifies their
# entire chains first, so an unchanged child seal cannot hide a broken parent.
manifest(){ printf 'key %s\n' "$K"; (cd "$D" && sha256sum "$@"); }
valid(){
 local phase=$1 actual recorded f
 case "$phase" in
  gen) set -- gen;;
  c-json) valid gen || return 1;set -- gen.seal c.json;;
  p-json) valid c-json || return 1;set -- c-json.seal model.json;;
  tbl) valid c-json && valid p-json || return 1;set -- c-json.seal p-json.seal model.tbl;;
  net) valid tbl || return 1;set -- tbl.seal model.net;;
  runner) valid net || return 1;set -- net.seal runner;;
  *) return 1;;
 esac
 [ -f "$D/$phase.seal" ] && [ ! -L "$D/$phase.seal" ] || return 1
 for f in "$@";do [ -f "$D/$f" ] && [ ! -L "$D/$f" ] && [ -s "$D/$f" ] || return 1;done
 case "$phase" in gen) [ -x "$D/gen" ] || return 1;; runner) [ -x "$D/runner" ] || return 1;;esac
 actual=$(manifest "$@") || return 1
 recorded=$(cat "$D/$phase.seal") || return 1
 [ "$actual" = "$recorded" ]
}
prerequisite(){
 case "$1" in
  gen) return 0;;
  c-json) valid gen || need_prepare --prepare-gen;;
  p-json) valid c-json || need_prepare --prepare-c-json;;
  tbl) valid c-json && valid p-json || need_prepare '--prepare-c-json then --prepare-p-json';;
  net) valid tbl || need_prepare --prepare-tbl;;
  runner) valid net || need_prepare --prepare-net;;
  *) return 2;;
 esac
}
seal(){
 local phase=$1
 case "$phase" in
  gen) set -- gen;;
  c-json) set -- gen.seal c.json;;
  p-json) set -- c-json.seal model.json;;
  tbl) set -- c-json.seal p-json.seal model.tbl;;
  net) set -- tbl.seal model.net;;
  runner) set -- net.seal runner;;
  *) exit 2;;
 esac
 manifest "$@" > "$T/$phase.seal"
 mv "$T/$phase.seal" "$D/$phase.seal"
}
prepare_stage(){
 local phase=$1
 if valid "$phase";then echo "libraryfunctionimports: $phase already prepared $K";return 0;fi
 prerequisite "$phase" || return $?
 echo "libraryfunctionimports: begin $phase $K"
 T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfunctionimports-$phase.XXXXXX")
 case "$phase" in
  gen) b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen";;
  c-json) b "$D/gen" parse2 "$T/c.json";;
  # This independent oracle hit the old20s bound in an isolated phase; keep it below the outer41s unit.
  p-json) "$R/tests/bound" 40 python3 exec/build/gen.py parse2 "$T/model.json";;
  tbl) b cmp "$D/c.json" "$D/model.json";b python3 exec/c/tbl.py "$D/model.json" "$T/model.tbl";;
  net) b python3 exec/c/net.py "$D/model.tbl" "$T/model.net";;
  runner) b cc -O2 -w exec/parse2/libraryfunctionimports-observer.c -o "$T/runner";;
 esac
 [ "$(key)" = "$K" ] || { echo 'libraryfunctionimports: preparation inputs changed' >&2;return 2; }
 prerequisite "$phase" || return $?
 # Publish data before its seal. The exclusive cache lock prevents consumers
 # from reading a mixed generation. A crash leaves invalid bytes, never PASS.
 case "$phase" in
  gen) mv "$T/gen" "$D/gen";;
  c-json) mv "$T/c.json" "$D/c.json";;
  p-json) mv "$T/model.json" "$D/model.json";;
  tbl) mv "$T/model.tbl" "$D/model.tbl";;
  net) mv "$T/model.net" "$D/model.net";;
  runner) mv "$T/runner" "$D/runner";;
 esac
 seal "$phase"
 valid "$phase" || { echo 'libraryfunctionimports: new seal/artifact verification failed' >&2;return 2; }
 sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T";T=
 echo "libraryfunctionimports: prepared $phase $K"
}
case "$mode" in
 check|--require-cached) [ -d "$D" ] && [ ! -L "$D" ] || { need_prepare --prepare-gen;exit 75; };;
 *) [ ! -L "$D" ] || { echo 'libraryfunctionimports: cache must not be a symlink' >&2;exit 2; };mkdir -p "$D";;
esac
# Formal atomic jobs must follow gen -> c-json -> p-json -> tbl -> net -> runner.
# No polling/implicit construction: a busy/stale lock is a refusal for the caller
# to inspect, never a claim that preparation completed.
mkdir "$D/lock" 2>/dev/null || { echo 'libraryfunctionimports: cache busy or stale lock; refusing' >&2;exit 75; }
LOCKED=1
case "$mode" in
 --prepare) prepare_stage gen;prepare_stage c-json;prepare_stage p-json;prepare_stage tbl;prepare_stage net;prepare_stage runner;exit 0;;
 --prepare-gen) prepare_stage gen;exit 0;;
 --prepare-c-json) prepare_stage c-json;exit 0;;
 --prepare-p-json) prepare_stage p-json;exit 0;;
 --prepare-tbl) prepare_stage tbl;exit 0;;
 --prepare-net) prepare_stage net;exit 0;;
 --prepare-runner) prepare_stage runner;exit 0;;
esac
valid runner || { need_prepare 'all atomic stages through --prepare-runner';exit 75; }
T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfunctionimports-check.XXXXXX")
cp "$D/model.net" "$D/model.tbl" "$D/runner" "$T/";cp tests/libraryfunctionimports-cases.tsv "$T/cases.tsv"
b cmp "$D/model.net" "$T/model.net";b cmp "$D/model.tbl" "$T/model.tbl";b cmp "$D/runner" "$T/runner"
valid runner || { need_prepare 'all atomic stages through --prepare-runner';exit 75; }
rmdir "$D/lock";LOCKED=0
"$R/tests/bound" 35 sh "${MODEL_COM:-./unisacc.com}" -run tests/libraryfunctionimportscheck.cx -- "$T"
