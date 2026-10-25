#!/bin/sh
# Reader+overlay: staged preparation prevents hiding cold construction in a 58s job.
set -eu
# Consumption is strictly prepared-only; this explicit flag cannot prepare.
if [ "${1:-}" = --require-cached ]; then
 shift
 case ${1:-check} in --*) echo 'require-cached: prepare options forbidden' >&2; exit 2;; esac
fi
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
b(){ "$R/tests/bound" 55 "$@"; }
UA=${MODEL_COM:-$R/unisacc.com}
K=$({ rg --files exec seed unisa include | rg '\.(tsv|py|c|h|sh|inc|json)$' | LC_ALL=C sort | xargs sha256sum; sha256sum tests/librarytypedreadercheck.cx tests/librarytypedreader-cases.tsv tests/librarytypedoverlaycheck.cx tests/librarytypedoverlay-ordinary.tsv; cc --version; printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1)
D=${TMPDIR:-/tmp}/unisacc-librarytypedreader-$K
mkdir -p "$D"
check(){ b sh "$UA" -run tests/librarytypedreadercheck.cx -- "$@"; }
prepare_typed_c(){
 (cd "$D" && sha256sum -c --status tools.sha256)
 b "$D/gen" parse2/librarytyped "$D/typed-c.json" --librarytyped --locations
 (cd "$D" && sha256sum tools.sha256 typed-c.json >typed-c.sha256)
}
prepare_typed_p(){
 (cd "$D" && sha256sum -c --status tools.sha256 && sha256sum -c --status typed-c.sha256)
 b python3 exec/build/gen.py parse2/librarytyped "$D/typed.json" --librarytyped --locations
 check --same "$D/typed-c.json" "$D/typed.json" >"$D/typed-cp.log"
 (cd "$D" && sha256sum typed-c.sha256 typed-c.json typed.json typed-cp.log >typed-p.sha256)
}
prepare_typed_tables(){
 (cd "$D" && sha256sum -c --status tools.sha256 && sha256sum -c --status typed-c.sha256 && sha256sum -c --status typed-p.sha256)
 b python3 exec/c/tbl.py "$D/typed.json" "$D/model.tbl"
 (cd "$D" && sha256sum typed-p.sha256 model.tbl >typed-tables.sha256)
}
prepare_typed_net(){
 (cd "$D" && sha256sum -c --status tools.sha256 && sha256sum -c --status typed-c.sha256 && sha256sum -c --status typed-p.sha256 && sha256sum -c --status typed-tables.sha256)
 b python3 exec/c/net.py "$D/model.tbl" "$D/model.net"
 (cd "$D" && sha256sum typed-tables.sha256 model.net >typed-net.sha256)
 (cd "$D" && sha256sum typed-c.json typed.json model.tbl model.net >typed.sha256)
}
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";;
--prepare-tools)
 b cc -std=c99 -O2 -Iseed seed/gen.c -o "$D/gen"
 b cc -O2 -w exec/parse2/librarytypedreader-observer.c -o "$D/runner"
 (cd "$D" && sha256sum gen runner >tools.sha256)
 ;;
--prepare-base)
 echo "reader: use --prepare-base-c, -off, -reference, -tables-base, -tables-off, -net-base, -net-off separately (each bounded to 58s)" >&2
 exit 2
 ;;
--prepare-base-c)
 (cd "$D" && sha256sum -c --status tools.sha256)
 b "$D/gen" parse2/librarytyped "$D/off-c.json"
 (cd "$D" && sha256sum off-c.json >base-c.sha256)
 ;;
--prepare-base-off)
 (cd "$D" && sha256sum -c --status base-c.sha256)
 b python3 exec/build/gen.py parse2/librarytyped "$D/off.json"
 check --same "$D/off-c.json" "$D/off.json" >"$D/off-cp.log"
 (cd "$D" && sha256sum off.json >base-off.sha256)
 ;;
--prepare-base-reference)
 (cd "$D" && sha256sum -c --status base-off.sha256)
 T=$(mktemp -d "${TMPDIR:-/tmp}/reader-base.XXXXXX")
 trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge --max-files 10000 -- "$T"' EXIT HUP INT TERM   # T holds a copy of exec/ (~1700 entries)
 cp -R exec "$T/exec"
 check --baseline "$T/exec/parse2/libraryimports-manifest.tsv"
 (cd "$T" && b python3 exec/build/gen.py parse2 "$D/base.json")
 check --same "$D/base.json" "$D/off.json" >"$D/base-off-json.log"
 (cd "$D" && sha256sum base.json >base-reference.sha256)
 ;;
--prepare-base-tables-base)
 (cd "$D" && sha256sum -c --status base-reference.sha256)
 b python3 exec/c/tbl.py "$D/base.json" "$D/base.tbl"
 (cd "$D" && sha256sum base.tbl >base-tables-base.sha256)
 ;;
--prepare-base-tables-off)
 (cd "$D" && sha256sum -c --status base-off.sha256 && sha256sum -c --status base-tables-base.sha256)
 b python3 exec/c/tbl.py "$D/off.json" "$D/off.tbl"
 check --same "$D/base.tbl" "$D/off.tbl" >"$D/base-off-tbl.log"
 (cd "$D" && sha256sum off.tbl >base-tables-off.sha256)
 ;;
--prepare-base-net-base)
 (cd "$D" && sha256sum -c --status base-tables-base.sha256)
 b python3 exec/c/net.py "$D/base.tbl" "$D/base.net"
 (cd "$D" && sha256sum base.net >base-net-base.sha256)
 ;;
--prepare-base-net-off)
 (cd "$D" && sha256sum -c --status base-tables-off.sha256 && sha256sum -c --status base-net-base.sha256)
 b python3 exec/c/net.py "$D/off.tbl" "$D/off.net"
 check --same "$D/base.net" "$D/off.net" >"$D/base-off-net.log"
 (cd "$D" && sha256sum -c --status base-c.sha256 && sha256sum -c --status base-off.sha256 && sha256sum -c --status base-reference.sha256 && sha256sum -c --status base-tables-base.sha256 && sha256sum -c --status base-tables-off.sha256 && sha256sum -c --status base-net-base.sha256 && sha256sum base.json base.tbl base.net off.json off.tbl off.net off-c.json >base.sha256)
 ;;
--prepare-audit)
 (cd "$D" && sha256sum -c --status tools.sha256)
 b "$D/gen" librarytypedpartition/gen "$D/audit-c.json"
 b python3 exec/build/gen.py librarytypedpartition/gen "$D/audit.json"
 check --same "$D/audit-c.json" "$D/audit.json"
 b python3 exec/c/tbl.py "$D/audit.json" "$D/audit.tbl"
 b python3 exec/c/net.py "$D/audit.tbl" "$D/audit.net"
 (cd "$D" && sha256sum audit-c.json audit.json audit.tbl audit.net >audit.sha256)
 ;;
--prepare-typed-c) prepare_typed_c ;;
--prepare-typed-p) prepare_typed_p ;;
--prepare-typed-tables) prepare_typed_tables ;;
--prepare-typed-net) prepare_typed_net ;;
--prepare-typed)
 prepare_typed_c
 prepare_typed_p
 prepare_typed_tables
 prepare_typed_net
 ;;
check)
 for phase in tools base typed audit;do
  if ! (cd "$D" && sha256sum -c --status "$phase.sha256");then
   echo "reader: preparation unavailable/corrupt; run the preparation steps listed in exec/parse2/librarytypedreader.md as separate bounded commands" >&2;exit 2
  fi
 done
 T=$(mktemp -d "${TMPDIR:-/tmp}/librarytypedreader-check.XXXXXX")
 trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT HUP INT TERM
 cp "$D/runner" "$D/model.tbl" "$D/model.net" "$D/audit.net" "$T/"
 cp tests/librarytypedreader-cases.tsv "$T/cases.tsv"
 mkdir "$T/A" "$T/B"
 check "$T"
 b sh "$UA" -run tests/librarytypedoverlaycheck.cx -- "$T"
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
