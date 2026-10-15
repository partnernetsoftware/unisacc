#!/bin/sh
# Reader only: staged preparation prevents hiding cold construction in a 58s job.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
b(){ "$R/tests/bound" 55 "$@"; }
UA=${MODEL_COM:-$R/unisacc.com}
K=$({ rg --files exec seed unisa include | rg '\.(tsv|py|c|h|sh|inc|json)$' | LC_ALL=C sort | xargs sha256sum; sha256sum tests/librarytypedreadercheck.cx tests/librarytypedreader-cases.tsv; cc --version; printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1)
D=${TMPDIR:-/tmp}/unisacc-librarytypedreader-$K
mkdir -p "$D"
check(){ b sh "$UA" -run tests/librarytypedreadercheck.cx -- "$@"; }
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";;
--prepare-tools)
 b cc -std=c99 -O2 -Iseed seed/gen.c -o "$D/gen"
 b cc -O2 -w exec/parse2/librarytypedreader-observer.c -o "$D/runner"
 (cd "$D" && sha256sum gen runner >tools.sha256)
 ;;
--prepare-base)
 (cd "$D" && sha256sum -c --status tools.sha256)
 b "$D/gen" parse2/librarytyped "$D/off-c.json"
 b python3 exec/build/gen.py parse2/librarytyped "$D/off.json"
 check --same "$D/off-c.json" "$D/off.json" >"$D/off-cp.log"
 T=$(mktemp -d "${TMPDIR:-/tmp}/reader-base.XXXXXX")
 trap 'rm -rf "$T"' EXIT HUP INT TERM
 cp -R exec "$T/exec"
 check --baseline "$T/exec/parse2/libraryimports-manifest.tsv"
 (cd "$T" && b python3 exec/build/gen.py parse2 "$D/base.json")
 check --same "$D/base.json" "$D/off.json" >"$D/base-off-json.log"
 b python3 exec/c/tbl.py "$D/base.json" "$D/base.tbl"
 b python3 exec/c/tbl.py "$D/off.json" "$D/off.tbl"
 check --same "$D/base.tbl" "$D/off.tbl" >"$D/base-off-tbl.log"
 b python3 exec/c/net.py "$D/base.tbl" "$D/base.net"
 b python3 exec/c/net.py "$D/off.tbl" "$D/off.net"
 check --same "$D/base.net" "$D/off.net" >"$D/base-off-net.log"
 (cd "$D" && sha256sum base.json base.tbl base.net off.json off.tbl off.net off-c.json >base.sha256)
 ;;
--prepare-typed)
 (cd "$D" && sha256sum -c --status tools.sha256)
 b "$D/gen" parse2/librarytyped "$D/typed-c.json" --librarytyped --locations
 b python3 exec/build/gen.py parse2/librarytyped "$D/typed.json" --librarytyped --locations
 check --same "$D/typed-c.json" "$D/typed.json" >"$D/typed-cp.log"
 b python3 exec/c/tbl.py "$D/typed.json" "$D/model.tbl"
 b python3 exec/c/net.py "$D/model.tbl" "$D/model.net"
 (cd "$D" && sha256sum typed-c.json typed.json model.tbl model.net >typed.sha256)
 ;;
check)
 for phase in tools base typed;do
  if ! (cd "$D" && sha256sum -c --status "$phase.sha256");then
   echo "reader: preparation unavailable/corrupt; run --prepare-tools, --prepare-base, --prepare-typed as separate bounded commands" >&2;exit 2
  fi
 done
 T=$(mktemp -d "${TMPDIR:-/tmp}/librarytypedreader-check.XXXXXX")
 trap 'rm -rf "$T"' EXIT HUP INT TERM
 cp "$D/runner" "$D/model.tbl" "$D/model.net" "$T/"
 cp tests/librarytypedreader-cases.tsv "$T/cases.tsv"
 mkdir "$T/A" "$T/B"
 check "$T"
 # parse2base.check must reject repeated P definitions in both constructors.
 set +e
 b "$D/gen" parse2/librarytypedduplicate "$T/duplicate-c.json" >"$T/duplicate-c.out" 2>"$T/duplicate-c.err";cr=$?
 b python3 exec/build/gen.py parse2/librarytypedduplicate "$T/duplicate-p.json" >"$T/duplicate-p.out" 2>"$T/duplicate-p.err";pr=$?
 set -e
 check --refusal "$T/duplicate-c.out" "$T/duplicate-c.err" "$cr" 'defined twice'
 check --refusal "$T/duplicate-p.out" "$T/duplicate-p.err" "$pr" 'defined twice'
 ;;
*) exit 2;;
esac
