#!/bin/sh
# B4: seed/gen.c builds all eight parse2 variants byte-identical to exec/build/gen.py.
# One command, one build, variants four at a time; a gen.c edit reruns only the C side (~13 s a variant).
# The Python references are cached under the seedmemory reference key: sha256 over the path and content of
# every file the measured gen.py parse2 route reads (tests/seedmemory-reads.tsv, strace-measured, computed by
# tests/seedmemorykey.cx), so an edit to any of them -- exec/finite_rules.py, exec/facts/load.py, exec/parse/*,
# exec/model*.tsv included -- misses the cache, and the admission (tests/seedmemory.cx) finds the warm references
# under the same name.  (0.0.43: the old key hashed exec/assemble.py, build, parse2 and facts/*.tsv only.)
# On a difference it names the first states only one side has.
#   usage: tests/seedparse2check.sh [FLAGSET...]
#          tests/seedparse2check.sh --key             print the cache key
#          tests/seedparse2check.sh --key-selftest    negative proof in private copies of the read set: a copy
#                                                     keys the same, an edit to a probed input misses the cache,
#                                                     a missing input gives no key
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
GEN_CC_FLAGS='-std=c99 -O2 -Iseed'
GEN_PARALLELISM=4
B=$R/tests/bound
cachekey() {   # cachekey DIR: the parse2 read-set key of the files under DIR; rc 1 when a listed file is missing
    k=$(cd "$1" && "$B" 30 sh "$R/unisacc.com" -run "$R/tests/seedmemorykey.cx" -- parse2) || return 1
    case $k in "parse2 "[0-9a-f]*) echo "${k#parse2 }";; *) return 1;; esac
}
refpath() { echo "$1/py-$2-$3.json"; }   # refpath CACHE KEY VARIANT: where run() looks for the Python reference
if [ "${1:-}" = --key-selftest ]; then
    W=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-seedparse2-key.XXXXXX") || exit 2; trap '[ -z "$W" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge --max-files 20000 -- "$W"' EXIT   # seven copies of the read set: a site ceiling
    awk -F '\t' 'NR > 1 && $1 == "parse2" { print $2 }' tests/seedmemory-reads.tsv > "$W/list"
    { cat "$W/list"; echo tests/seedmemory-reads.tsv; } > "$W/files"
    p=$(grep -m1 '^exec/parse/' "$W/list"); m=$(grep -m1 '^exec/model' "$W/list")
    probes="exec/finite_rules.py exec/facts/load.py ${p:-exec/parse/NONE} ${m:-exec/model-NONE} exec/parse2/gen-manifest.tsv"
    i=0; for c in same $probes gone; do   # one private copy per case, keyed in parallel
        mkdir "$W/$i" && tar -cf - -T "$W/files" | (cd "$W/$i" && tar -xf -) || { echo "seedparse2-key: cannot copy the read set"; exit 2; }
        case $c in same) ;; gone) sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$W/$i/exec/finite_rules.py" || exit 3;; *) [ -f "$W/$i/$c" ] && printf '\n' >> "$W/$i/$c";; esac
        { cachekey "$W/$i" || echo none; } > "$W/k$i" & i=$((i + 1))
    done
    k0=$(cachekey "$R") || { echo "seedparse2-key: no key for the checkout"; exit 1; }; wait
    mkdir "$W/cache"; echo '{}' > "$(refpath "$W/cache" "$k0" x)"   # a warm reference under the checkout's key
    bad=0; i=0; n=0
    for c in same $probes gone; do
        k=$(cat "$W/k$i"); i=$((i + 1))
        case $c in
          same) [ "$k" = "$k0" ] && echo "SAME   copy keys as the checkout $k0" || { echo "BAD    copy keys $k, checkout $k0"; bad=1; };;
          gone) [ "$k" = none ] && echo "NOKEY  without exec/finite_rules.py" || { echo "BAD    key $k without exec/finite_rules.py"; bad=1; };;
          *) if ! grep -qx "$c" "$W/list"; then echo "BAD    $c is not in the measured read set"; bad=1
             elif [ "$k" != none ] && [ "$k" != "$k0" ] && [ ! -e "$(refpath "$W/cache" "$k" x)" ]; then echo "MISS   $c edited: key $k"; n=$((n + 1))
             else echo "BAD    $c edited: key $k still hits the cache"; bad=1; fi;;
        esac
    done
    echo "seedparse2-key  missed $n  bad $bad"; [ "$bad" = 0 ] && [ "$n" -gt 0 ]; exit
fi
key=$(cachekey "$R") || { echo "seedparse2: no cache key (a file of tests/seedmemory-reads.tsv parse2 is missing)"; exit 2; }
[ "${1:-}" = --key ] && { echo "$key"; exit 0; }
sh "$R/tests/cxbin" "$R/tests/seedmemory.cx" parse2 "$@" || exit $?
T=${TMPDIR:-/tmp}/unisacc-seedparse2; mkdir -p "$T"
"$B" 55 cc $GEN_CC_FLAGS -o "$T/gen" seed/gen.c || { echo "seedparse2: gen.c does not build"; exit 1; }
sh "$R/tests/cxbin" "$R/tests/seedmemory.cx" parse2 "$@" --verify-binary "$T/gen" || exit $?
sets=${*:-"x locations warnings errors locations,warnings locations,errors warnings,errors locations,warnings,errors"}
run() {   # one variant: C now, Python from cache
    v=$1; fl=$(echo "$v" | tr ',' ' ' | sed 's/x//; s/\([a-z][a-z]*\)/--\1/g')
    py=$(refpath "$T" "$key" "$v")
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
