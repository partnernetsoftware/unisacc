#!/bin/sh
# B4: seed/gen.c builds all eight parse2 variants byte-identical to exec/build/gen.py.
# One command, one build, variants four at a time; the Python references are cached by the hash of
# their inputs (exec/ minus seed), so a gen.c edit reruns only the C side (~13 s a variant).
# On a difference it names the first states only one side has.   usage: tests/seedparse2check.sh [FLAGSET...]
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
GEN_CC_FLAGS='-std=c99 -O2 -Iseed'
GEN_PARALLELISM=4
python3 "$R/tests/seedmemory.py" parse2 "$@" || exit $?
B=$R/tests/bound
T=${TMPDIR:-/tmp}/unisacc-seedparse2; mkdir -p "$T"
key=$(cat exec/assemble.py exec/build/*.py exec/parse2/* exec/facts/*.tsv 2>/dev/null | shasum | cut -c1-16)
"$B" 55 cc $GEN_CC_FLAGS -o "$T/gen" seed/gen.c || { echo "seedparse2: gen.c does not build"; exit 1; }
python3 "$R/tests/seedmemory.py" parse2 "$@" --verify-binary "$T/gen" || exit $?
sets=${*:-"x locations warnings errors locations,warnings locations,errors warnings,errors locations,warnings,errors"}
run() {   # one variant: C now, Python from cache
    v=$1; fl=$(echo "$v" | tr ',' ' ' | sed 's/x//; s/\([a-z][a-z]*\)/--\1/g')
    py="$T/py-$key-$v.json"
    [ -s "$py" ] || "$B" 55 python3 exec/build/gen.py parse2 "$py.tmp" $fl >/dev/null 2>&1 && mv "$py.tmp" "$py" 2>/dev/null
    "$B" 55 "$T/gen" parse2 "$T/c-$v.json" $fl 2> "$T/c-$v.err"
    if cmp -s "$T/c-$v.json" "$py"; then echo "SAME $v"
    else echo "DIFF $v $(tail -1 "$T/c-$v.err")"; python3 - "$T/c-$v.json" "$py" <<'P'
import json, sys
try: c, p = (json.load(open(f)) for f in sys.argv[1:3])
except Exception as e: print("   (unreadable: %s)" % e); sys.exit()
sc, sp = (set(x.get('states', x).keys()) if isinstance(x.get('states', x), dict) else set() for x in (c, p))
print("   only C:", sorted(sc - sp)[:5], " only Python:", sorted(sp - sc)[:5])
P
    fi
}
n=0; for v in $sets; do run "$v" > "$T/r-$v.txt" & n=$((n+1)); [ $((n % GEN_PARALLELISM)) = 0 ] && wait; done; wait
same=0; diff=0; for v in $sets; do cat "$T/r-$v.txt"; grep -q '^SAME' "$T/r-$v.txt" && same=$((same+1)) || diff=$((diff+1)); done
echo "seedparse2  same $same  differ $diff"; [ "$diff" = 0 ]
