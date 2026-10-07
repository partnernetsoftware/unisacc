#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Development route: C source -> ELF, Mach-O or PE, seven deltas on one generic C executor.
# Python constructs models only; after that no Python stage processes a source.
# NETWORK=0 selects the reference lookup-table execution; default is inference.
# Linux/macOS x86_64 or arm64, Windows x86_64 or arm64; frontend coverage limits apply.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -ge 2 ] || { echo 'usage: elf.sh OUTPUT_DIR FILE.c...' >&2; exit 2; }
TARGET=${TARGET:-lnx/x86_64}
case $TARGET in lnx/x86_64|lnx/arm64|osx/x86_64|osx/arm64|win/arm64|win/x86_64) ;; *) echo "unsupported target: $TARGET" >&2; exit 2;; esac
OUT=$1; shift
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
b() { "$_BOUND" 60 "$@"; }
if [ "${STAGE_MODELS_READY:-0}" != 1 ]; then
    b python3 exec/pipeline/models.py "$OUT" "$TARGET" "${NETWORK:-1}" "${EXEC_CC:-cc}"
fi
case $TARGET in win/*) IMAGE=pe;; osx/*) IMAGE=macho;; *) IMAGE=elf;; esac
case ${NETWORK:-1} in 0) MODEL=tbl;; 1) MODEL=net;; *) exit 2;; esac
STAGES=$(awk '!/^#/ && NF {print $1}' exec/pipeline/image-stages.tsv)
STAGE_FROM=${STAGE_FROM:-e2}; STAGE_TO=${STAGE_TO:-elf}
case $STAGE_FROM:$STAGE_TO in
    e2:*|e1:*|e3:*|e4:*|prune:*|lower:*|elf:*) ;;
    *) echo "unknown start stage: $STAGE_FROM" >&2; exit 2;;
esac
case $STAGE_TO in e2|e1|e3|e4|prune|lower|elf) ;; *) echo "unknown end stage: $STAGE_TO" >&2; exit 2;; esac
case $IMAGE in pe) EXT=exe;; *) EXT=$IMAGE;; esac
: > "$OUT/inputs"
for f in "$@"; do
    name=$(basename "$f" .c)
    if grep -Fxq "$name" "$OUT/inputs"; then echo "duplicate output name: $name" >&2; exit 2; fi
    printf '%s\n' "$name" >> "$OUT/inputs"
    export UNISA_ATTRIBUTES="$OUT/$name.attributes"
    if [ "$STAGE_FROM" = e2 ]; then rm -f "$UNISA_ATTRIBUTES"; fi
    in=$f; previous=; running=0; finished=0
    for s in $STAGES; do
        if [ "$s" = "$STAGE_FROM" ]; then
            running=1
            if [ -n "$previous" ]; then
                in="$OUT/$name.$previous"
                [ -s "$in" ] || { echo "missing preceding stage: $in" >&2; exit 1; }
            fi
        fi
        if [ "$running" = 1 ]; then
            out="$OUT/$name.$s"
            case $s in
                e2) b "$OUT/run" "$OUT/$s.$MODEL" "$in" "$f" "$R/include" > "$out" ;;
                e4) b env UNISA_MAXSTEPS=400000000000 "$OUT/run" "$OUT/$s.$MODEL" "$in" "$f" > "$out" ;; # same step budget as exec/opt/check.sh; wall bound stays 60 s
                *) b "$OUT/run" "$OUT/$s.$MODEL" "$in" "$f" > "$out" ;;
            esac
            in=$out
            if [ "$s" = "$STAGE_TO" ]; then finished=1; break; fi
        fi
        previous=$s
    done
    [ "$finished" = 1 ] || { echo "stage range is empty or reversed: $STAGE_FROM..$STAGE_TO" >&2; exit 2; }
    if [ "$STAGE_TO" = elf ]; then
        if [ "$EXT" != elf ]; then mv "$OUT/$name.elf" "$OUT/$name.$EXT"; fi
        chmod +x "$OUT/$name.$EXT"
        echo "$MODEL $IMAGE: $f -> $OUT/$name.$EXT"
    else
        echo "$MODEL $IMAGE: $f -> $OUT/$name.$STAGE_TO"
    fi
done
