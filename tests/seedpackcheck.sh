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
echo "seedpack  same $same  (P1, P2, P3 with mounts)  refusals 2"
