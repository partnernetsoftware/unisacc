#!/bin/sh
# 0.0.32 B5: seed/gen.c builds the product's shared deltas byte-identical to exec/build/gen.py --
# the stages buildcompiler.sh's `shared` step constructs (e2 pp --shared-predefines, e1 lex --typed,
# e3 parse2, e4 opt --o2, o1 opt, prune, nativeabi).  Python references are cached by the hash of
# exec/ inputs, so a gen.c edit reruns only the C side.     usage: tests/seedgencheck.sh [NAME...]
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=${TMPDIR:-/tmp}/unisacc-seedgen; mkdir -p "$T"
key=$(cat exec/assemble.py exec/finite_rules.py exec/build/*.py exec/*/*.tsv exec/facts/*.tsv weights/gold/*.tsv 2>/dev/null | shasum | cut -c1-16)
"$B" 55 cc -std=c99 -O2 -Iseed -o "$T/gen" seed/gen.c || { echo "seedgen: gen.c does not build"; exit 1; }
spec() { case $1 in
    e2) echo "pp --shared-predefines";; e1) echo "lex --typed";; e3) echo parse2;; e4) echo "opt --o2";;
    o1) echo opt;; prune) echo prune;; nativeabi) echo nativeabi;; *) echo "?";; esac; }
names=${*:-"e2 e1 e3 e4 o1 prune nativeabi"}
run() {
    n=$1; set -- $(spec "$n"); st=$1; shift
    [ "$st" = "?" ] && { echo "DIFF $n (unknown name)"; return; }
    py="$T/py-$key-$n.json"
    [ -s "$py" ] || { "$B" 55 python3 exec/build/gen.py "$st" "$py.tmp" "$@" >/dev/null 2>&1 && mv "$py.tmp" "$py"; }
    [ -s "$py" ] || { echo "DIFF $n (python reference failed)"; return; }
    "$B" 55 "$T/gen" "$st" "$T/c-$n.json" "$@" 2> "$T/c-$n.err"
    if cmp -s "$T/c-$n.json" "$py"; then echo "SAME $n"; else echo "DIFF $n $(tail -1 "$T/c-$n.err")"; fi
}
k=0; for n in $names; do run "$n" > "$T/r-$n.txt" & k=$((k+1)); [ $((k % 4)) = 0 ] && wait; done; wait
# seed/ident.c: the product source identity, equal to exec/c/provenance.py identity (B5)
if [ -z "$*" ]; then
    names="$names ident"
    { cc -std=c99 -D_POSIX_C_SOURCE=200809L -O2 -w seed/ident.c -o "$T/ident" &&
      a=$("$B" 30 "$T/ident") && p=$("$B" 30 python3 exec/c/provenance.py identity) && [ -n "$a" ] && [ "$a" = "$p" ] &&
      head -c 4096 /dev/urandom > "$T/art" && "$B" 30 "$T/ident" . write "$T/art" "$a" && cp "$T/art.build.json" "$T/art.c.json" &&
      "$B" 30 python3 exec/c/provenance.py write "$T/art" "$a" && cmp -s "$T/art.c.json" "$T/art.build.json" &&
      "$B" 30 "$T/ident" . check "$T/art" >/dev/null &&
      echo "SAME ident"; } > "$T/r-ident.txt" 2>&1 || echo "DIFF ident" >> "$T/r-ident.txt"
fi
same=0; diff=0; for n in $names; do cat "$T/r-$n.txt"; grep -q '^SAME' "$T/r-$n.txt" && same=$((same+1)) || diff=$((diff+1)); done
echo "seedgen  same $same  differ $diff"; [ "$diff" = 0 ] && [ "$same" -gt 0 ]
