#!/bin/sh
# 0.0.32 B5: seed/pack.c writes the compiler package byte-identical to exec/c/pack.py -- P1, P2 and
# the shipped P3 (UNINETB1 + raw DEFLATE 9 + CRC through zlib), Q-prefix compaction, network dedup,
# and resource mounts in PurePath component order (a-b vs a/b) with first-wins duplicates.
# Networks come from seed-gen/seed-tbl/seed-net on small stages.     usage: tests/seedpackcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-seedpack.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "seedpack: $*"; exit 1; }
"$B" 50 cc -std=c99 -O2 -w -o "$T/pack" seed/pack.c -lz || fail "pack.c does not build"
"$B" 50 cc -std=c99 -O2 -w -Iseed -o "$T/gen" seed/gen.c || fail "gen.c does not build"
"$B" 50 cc -std=c99 -O2 -w -o "$T/tbl" seed/tbl.c && "$B" 50 cc -std=c99 -O2 -w -o "$T/net" seed/net.c || fail "tbl/net do not build"
for s in prune nativeabi; do
    "$B" 50 "$T/gen" $s "$T/$s.json" && "$B" 50 "$T/tbl" "$T/$s.json" "$T/$s.tbl" && "$B" 50 "$T/net" "$T/$s.tbl" "$T/$s.net" >/dev/null || fail "$s network"
done
printf 'r1\tprune\ttape\ttape\tprune.net\nr1\tabi\ttape\tabi.v1\tnativeabi.net\n# comment\n\nr2\tprune\ttape\ttape\tprune.net\n' > "$T/routes.tsv"
mkdir -p "$T/m/a" "$T/m/a-b" "$T/n"; printf x > "$T/m/a/b"; printf y > "$T/m/a-b/c"; printf z > "$T/m/a.c"; printf w > "$T/m/a0"
printf x > "$T/n/b"
same=0
for mode in "" "--mount 006d2f $T/m --mount 00 $T/n" "--compressed --mount 006864722f include --mount 006d2f $T/m"; do
    # shellcheck disable=SC2086
    "$B" 50 python3 exec/c/pack.py --no-codec-cache $mode -o "$T/py.pkg" "$T/routes.tsv" >/dev/null || fail "pack.py failed: $mode"
    # shellcheck disable=SC2086
    "$B" 50 "$T/pack" $mode -o "$T/c.pkg" "$T/routes.tsv" >/dev/null || fail "pack.c failed: $mode"
    cmp -s "$T/py.pkg" "$T/c.pkg" || fail "differs: ${mode:-P1}"
    same=$((same+1))
done
printf 'r1\tprune\ttape\ttape\tprune.net\nr1\tprune\ttape\ttape\tprune.net\n' > "$T/dup.tsv"
"$B" 20 "$T/pack" -o "$T/x.pkg" "$T/dup.tsv" 2>/dev/null && fail "duplicate stage accepted"
printf 'r1\tprune\ttape\tx\tprune.net\nr1\tabi\ttape\tabi\tnativeabi.net\n' > "$T/fmt.tsv"
"$B" 20 "$T/pack" -o "$T/x.pkg" "$T/fmt.tsv" 2>/dev/null && fail "format mismatch accepted"
# seed/compilerpack.c: the route declaration of compilerpack.py (six targets, every route family,
# predefines, kernel checks, model audit), on stand-in networks: the rows, not the models, are tested
"$B" 50 cc -std=c99 -O2 -w -Iseed -o "$T/cpk" seed/compilerpack.c || fail "compilerpack.c does not build"
mkdir -p "$T/models" "$T/k" "$T/sh"
for n in object-lower-arm64 object-lower-x86_64 object-enc-arm64 object-enc-x86_64 tokenpp tokenlex warnlex warnparse warnunits errorparse warnpp-shared; do
    cp "$T/prune.net" "$T/models/$n.net"; cp "$T/prune.tbl" "$T/models/$n.tbl"
done
for n in e2 o1 nativeabi; do cp "$T/nativeabi.net" "$T/sh/$n.net"; cp "$T/nativeabi.tbl" "$T/sh/$n.tbl"; done
python3 -c 'import struct,sys
for isa,a in ((1,"arm64"),(2,"x86_64")): open(sys.argv[1]+"/"+a,"wb").write(b"UNIKERN1"+struct.pack("<4Q",isa,4,8,64)+bytes(range(8))+bytes(56))' "$T/k"
routes=
for t in lnx/arm64 lnx/x86_64 osx/arm64 osx/x86_64 win/arm64 win/x86_64; do
    d=$T/$(echo $t | tr / -); mkdir -p "$d"
    for st in e2 e1 e3 e4 prune lower elf; do
        src=prune; case $st in e1|e3|lower) src=nativeabi;; esac
        cp "$T/$src.net" "$d/$st.net"; cp "$T/$src.tbl" "$d/$st.tbl"; m=$st.net
        case $st in e2) f="src.c pp";; e1) f="pp tokens";; e3) f="tokens tape";; lower) f="tape target.text";; elf) f="target.text image";; *) f="tape tape";; esac
        printf '%s\t%s\t%s\t%s\t%s\n' "$t" "$st" ${f% *} ${f#* } "$m"
    done > "$d/route.tsv"
    routes="$routes $d/route.tsv"
done
# shellcheck disable=SC2086
"$B" 50 python3 - "$T" $routes <<'PY' >"$T/py.log" 2>&1 || { tail -3 "$T/py.log"; fail "compilerpack.py reference failed"; }
import sys, shutil, pathlib
sys.path.insert(0, 'exec/c')
import compilerpack as cp
T = pathlib.Path(sys.argv[1])
def bm(td, name, script, args, env=None):
    n = pathlib.Path(td)/(name+'.net'); shutil.copyfile(T/'models'/(name+'.net'), n); shutil.copyfile(T/'models'/(name+'.tbl'), n.with_suffix('.tbl')); return n
cp.built_model = bm; cp.MODEL_CACHE = False
p = cp.compiler_package(list(map(pathlib.Path, sys.argv[2:])), T/'sh/o1.net', pathlib.Path('include'), T/'k', T/'pyaudit', True, T/'sh/e2.net', T/'sh/nativeabi.net')
(T/'py-compiler.pkg').write_bytes(p)
PY
# shellcheck disable=SC2086
"$B" 50 "$T/cpk" "$T/cp" --o1 "$T/sh/o1.net" --e2 "$T/sh/e2.net" --nativeabi "$T/sh/nativeabi.net" --models "$T/models" --kernels "$T/k" --audit-dir "$T/caudit" $routes >/dev/null &&
"$B" 50 "$T/pack" --compressed -o "$T/c-compiler.pkg" --mount 006864722f include --mount 006b65726e656c2f "$T/k" --mount 00707265646566696e65732f "$T/cp/predefines" "$T/cp/routes.tsv" >/dev/null || fail "compilerpack.c failed"
cmp -s "$T/py-compiler.pkg" "$T/c-compiler.pkg" || fail "compiler package differs"
cmp -s "$T/pyaudit/models.json" "$T/caudit/models.json" && [ "$(ls "$T/pyaudit")" = "$(ls "$T/caudit")" ] || fail "model audit differs"
rows=$(wc -l < "$T/cp/routes.tsv" | tr -d ' ')
printf 'X' >> "$T/k/arm64"
# shellcheck disable=SC2086
"$B" 20 "$T/cpk" "$T/cp2" --o1 "$T/sh/o1.net" --e2 "$T/sh/e2.net" --nativeabi "$T/sh/nativeabi.net" --models "$T/models" --kernels "$T/k" $routes 2>/dev/null && fail "bad kernel extent accepted"
echo "seedpack  same $((same+1))  (P1, P2, P3 with mounts; compiler package $rows routes + audit)  refusals 3"
