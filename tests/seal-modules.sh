#!/bin/sh
# 封存范式模块独立测套：streamseal + netseal 闸回放（不绑 middle-flow / ledger 台账）。
# 真源：tests/SEAL-MODULES.md、tests/seal-modules.manifest.tsv
#
#   ./tests/seal-modules.sh              manifest+xy + 全部 gate 回放（内层并行）
#   ./tests/seal-modules.sh --check-only  仅结构/适用性，不跑 streamseal/netseal
#   ./tests/seal-modules.sh --stream-only | --grid-only
#
# 环境：MODEL_COM（默认 ./unisacc.com），JOBS（默认 4，内层并行 gate）
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
cd "$R" || exit 2
PRODUCT="${MODEL_COM:-./unisacc.com}"
BOUND="$R/tests/bound"
JOBS=${JOBS:-4}
MODE=all
case "${1:-}" in
    --check-only) MODE=check;;
    --stream-only) MODE=stream;;
    --grid-only) MODE=grid;;
    "") MODE=all;;
    -h|--help)
        echo "usage: tests/seal-modules.sh [--check-only|--stream-only|--grid-only]"
        exit 0;;
    *) echo "seal-modules: unknown option $1" >&2; exit 2;;
esac

python3 "$R/tests/sealmodulescheck.py" --check || exit 1
[ "$MODE" = check ] && { echo "seal-modules: check-only ok"; exit 0; }

TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT

list_jobs() {
    case "$MODE" in
        stream) awk -F '\t' '$1=="stream" {print "stream|"$3"|tests/streamseal.cx"}' "$R/tests/seal-modules.manifest.tsv";;
        grid)   awk -F '\t' '$1=="grid" {print "grid|"$3"|tests/netseal.cx"}' "$R/tests/seal-modules.manifest.tsv";;
        all)
            awk -F '\t' '$1=="stream" {print "stream|"$3"|tests/streamseal.cx"}
                 $1=="grid" {print "grid|"$3"|tests/netseal.cx"}' "$R/tests/seal-modules.manifest.tsv";;
    esac
}

n=0
while IFS= read -r line; do
    [ -n "$line" ] || continue
    n=$((n + 1))
    printf '%s\n' "$line" >> "$TMP/jobs.txt"
done <<EOF
$(list_jobs)
EOF
[ "$n" -gt 0 ] || { echo "seal-modules: nothing to run"; exit 2; }

echo "seal-modules: ${n} gate job(s), JOBS=$JOBS product=$PRODUCT"
bad=0
slot=0
while IFS='|' read -r kind leaf checker; do
    [ -n "$kind" ] || continue
    while [ "$(ls "$TMP"/run.* 2>/dev/null | wc -l | tr -d ' ')" -ge "$JOBS" ]; do
        for f in "$TMP"/run.*; do
            [ -f "$f" ] || continue
            pid=${f##*.}
            if ! kill -0 "$pid" 2>/dev/null; then
                wait "$pid" || bad=$((bad + 1))
                rm -f "$f"
            fi
        done
        sleep 0.05
    done
    slot=$((slot + 1))
    (
        if "$BOUND" 58 sh "$PRODUCT" -run "$checker" -- --gate "$leaf" .; then
            echo "  ok   $kind $leaf"
        else
            echo "seal-modules: FAIL $kind gate $leaf" >&2
            exit 1
        fi
    ) &
    echo "$slot" > "$TMP/run.$!"
done < "$TMP/jobs.txt"

for f in "$TMP"/run.*; do
    [ -f "$f" ] || continue
    pid=${f##*.}
    wait "$pid" || bad=$((bad + 1))
done

if [ "$bad" -ne 0 ]; then
    echo "seal-modules: FAIL $bad gate job(s)" >&2
    exit 1
fi
echo "seal-modules: all ${n} gate job(s) ok"
exit 0
