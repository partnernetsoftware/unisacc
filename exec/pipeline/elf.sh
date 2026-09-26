#!/bin/sh
# Development route: C source -> ELF or Mach-O, six deltas on one generic C executor.
# Python generates tables only; after that no Python stage processes a source.
# Linux/macOS x86_64 or arm64; current frontend coverage limits still apply.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -ge 2 ] || { echo 'usage: elf.sh OUTPUT_DIR FILE.c...' >&2; exit 2; }
TARGET=${TARGET:-lnx/x86_64}
case $TARGET in lnx/x86_64|lnx/arm64|osx/x86_64|osx/arm64) ;; *) echo "unsupported target: $TARGET" >&2; exit 2;; esac
OUT=$1; shift
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$OUT/run" exec/c/run.c
b python3 exec/pp/gen.py "$OUT/e2.json" "$TARGET"
b python3 exec/lex/gen.py --typed "$OUT/e1.json"
b python3 exec/parse2/gen2.py "$OUT/e3.json"
b python3 exec/opt/gen.py "$OUT/e4.json" 2
case $TARGET in osx/*) OSFLAG=--osx; IMAGE=macho;; *) OSFLAG=; IMAGE=elf;; esac
case $TARGET in */arm64) ARCHFLAG=--arm64; ENCODER=arm.py;; *) ARCHFLAG=; ENCODER=gen.py;; esac
b python3 exec/lower/gen.py "$OUT/lower.json" --full $OSFLAG $ARCHFLAG
b python3 "exec/enc/$ENCODER" "$OUT/elf.json" "--$IMAGE"
for s in e2 e1 e3 e4 lower elf; do b python3 exec/c/tbl.py "$OUT/$s.json" "$OUT/$s.tbl"; done
: > "$OUT/inputs"
for f in "$@"; do
    name=$(basename "$f" .c)
    if grep -Fxq "$name" "$OUT/inputs"; then echo "duplicate output name: $name" >&2; exit 2; fi
    printf '%s\n' "$name" >> "$OUT/inputs"
    in=$f
    for s in e2 e1 e3 e4 lower elf; do
        out="$OUT/$name.$s"
        case $s in
            e2) b "$OUT/run" "$OUT/$s.tbl" "$in" "$f" "$R/include" > "$out" ;;
            e4) b env UNISA_MAXSTEPS=400000000000 "$OUT/run" "$OUT/$s.tbl" "$in" "$f" > "$out" ;; # same step budget as exec/opt/check.sh; wall bound stays 60 s
            *) b "$OUT/run" "$OUT/$s.tbl" "$in" "$f" > "$out" ;;
        esac
        in=$out
    done
    if [ "$IMAGE" != elf ]; then mv "$OUT/$name.elf" "$OUT/$name.$IMAGE"; fi
    chmod +x "$OUT/$name.$IMAGE"
    echo "delta $IMAGE: $f -> $OUT/$name.$IMAGE"
done
