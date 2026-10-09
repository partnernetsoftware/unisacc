#!/bin/sh
# 0.0.32 B5: seed/gen.c builds the product's shared deltas byte-identical to exec/build/gen.py --
# the stages buildcompiler.sh's `shared` step constructs (e2 pp --shared-predefines, e1 lex --typed,
# e3 parse2, e4 opt --o2, o1 opt, prune, nativeabi, enc per object format, lower --full per target, and compilerpack's model jobs).  Python references are cached by the hash of
# exec/ inputs, so a gen.c edit reruns only the C side.     usage: tests/seedgencheck.sh [NAME...]
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
GEN_CC_FLAGS='-std=c99 -O2 -Iseed'
GEN_PARALLELISM=4
python3 "$R/tests/seedmemory.py" gen "$@" || exit $?
B=$R/tests/bound
T=${TMPDIR:-/tmp}/unisacc-seedgen; mkdir -p "$T"
key=$(cat exec/assemble.py exec/finite_rules.py exec/build/*.py exec/*/*.tsv exec/facts/*.tsv weights/gold/*.tsv 2>/dev/null | shasum | cut -c1-16)
G=$T/gen.$$; trap 'rm -f "$G"' EXIT   # per run: gate shards build concurrently
"$B" 55 cc $GEN_CC_FLAGS -o "$G" seed/gen.c || { echo "seedgen: gen.c does not build"; exit 1; }
python3 "$R/tests/seedmemory.py" gen "$@" --verify-binary "$G" || exit $?
spec() { case $1 in
    e2) echo "pp --shared-predefines";; e1) echo "lex --typed";; e3) echo parse2;; e4) echo "opt --o2";;
    o1) echo opt;; prune) echo prune;; nativeabi) echo nativeabi;;
    enc-elf) echo "enc --elf";; enc-macho) echo "enc --macho";; enc-pe) echo "enc --pe";;
    arm-elf) echo "enc/arm --elf";; arm-macho) echo "enc/arm --macho";; arm-pe) echo "enc/arm --pe";;
    lower-lnx-x) echo "lower --full";; lower-lnx-a) echo "lower --full --arm64";;
    lower-osx-x) echo "lower --full --osx";; lower-osx-a) echo "lower --full --osx --arm64";;
    lower-win-x) echo "lower --full --win";; lower-win-a) echo "lower --full --win --arm64";;
    obj-lower-x) echo "lower --full --object";; obj-lower-a) echo "lower --full --object --arm64";;
    obj-enc-x) echo "enc --object";; obj-enc-a) echo "enc/arm --object";;
    tokenpp) echo "pp --shared-predefines --no-autoinc";; tokenlex) echo lex;; warnlex) echo "lex --locations";;
    warnparse) echo "parse2 --warnings --errors";; warnunits) echo "parse2/units --locations";;
    errorparse) echo "parse2 --errors";; warnpp) echo "pp --locations --shared-predefines";; *) echo "?";; esac; }
names=${*:-"e2 e1 e3 e4 o1 prune nativeabi enc-elf enc-macho enc-pe arm-elf arm-macho arm-pe lower-lnx-x lower-lnx-a lower-osx-x lower-osx-a lower-win-x lower-win-a obj-lower-x obj-lower-a obj-enc-x obj-enc-a tokenpp tokenlex warnlex warnparse warnunits errorparse warnpp"}
run() {
    n=$1; set -- $(spec "$n"); st=$1; shift
    [ "$st" = "?" ] && { echo "DIFF $n (unknown name)"; return; }
    py="$T/py-$key-$n.json"
    [ -s "$py" ] || { "$B" 55 python3 exec/build/gen.py "$st" "$py.tmp" "$@" >/dev/null 2>&1 && mv "$py.tmp" "$py"; }
    [ -s "$py" ] || { echo "DIFF $n (python reference failed)"; return; }
    # a fresh output each run, and only a zero exit may compare: a timed-out or failed
    # generator must not be judged by a c-$n.json some earlier run left behind
    rm -f "$T/c-$n.json"
    "$B" 55 "$G" "$st" "$T/c-$n.json.$$" "$@" 2> "$T/c-$n.err" || { echo "DIFF $n (C generator rc=$?) $(tail -1 "$T/c-$n.err")"; rm -f "$T/c-$n.json.$$"; return; }
    mv "$T/c-$n.json.$$" "$T/c-$n.json"
    if cmp -s "$T/c-$n.json" "$py"; then echo "SAME $n"; else echo "DIFF $n $(tail -1 "$T/c-$n.err")"; fi
}
k=0; for n in $names; do run "$n" > "$T/r-$n.txt" & k=$((k+1)); [ $((k % GEN_PARALLELISM)) = 0 ] && wait; done; wait
# seed/ident.c: the product source identity, equal to exec/c/provenance.py identity (B5)
if [ -z "$*" ] || [ "$1" = e2 ]; then   # the first gate shard also checks ident and blob
    names="$names ident"
    { cc -std=c99 -D_POSIX_C_SOURCE=200809L -O2 -w seed/ident.c -o "$T/ident" &&
      a=$("$B" 30 "$T/ident") && p=$("$B" 30 python3 exec/c/provenance.py identity) && [ -n "$a" ] && [ "$a" = "$p" ] &&
      head -c 4096 /dev/urandom > "$T/art" && "$B" 30 "$T/ident" . write "$T/art" "$a" && cp "$T/art.build.json" "$T/art.c.json" &&
      "$B" 30 python3 exec/c/provenance.py write "$T/art" "$a" && cmp -s "$T/art.c.json" "$T/art.build.json" &&
      "$B" 30 "$T/ident" . check "$T/art" >/dev/null &&
      echo "SAME ident"; } > "$T/r-ident.txt" 2>&1 || echo "DIFF ident" >> "$T/r-ident.txt"
    # seed/blob.c: the kernel blob, equal to exec/c/asm/blob.py (B5); native Apple or Linux LLVM seed tools
    # Both constructors use exec/c/asm/machocc.sh; no runtime fallback or cached blob.
    if [ "$(uname -s)" = Darwin ] || [ "$(uname -s)" = Linux ]; then
        names="$names blob"
        { cc -std=c99 -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -O2 -w seed/blob.c -o "$T/blob" &&
          for a in arm64 x86_64; do "$B" 50 "$T/blob" . $a "$T/k.c.$a" >/dev/null && "$B" 50 python3 exec/c/asm/blob.py $a "$T/k.py.$a" >/dev/null &&
            cmp -s "$T/k.c.$a" "$T/k.py.$a" || exit 1; done && echo "SAME blob"; } > "$T/r-blob.txt" 2>&1 || echo "DIFF blob" >> "$T/r-blob.txt"
    fi
fi
same=0; diff=0; for n in $names; do cat "$T/r-$n.txt"; grep -q '^SAME' "$T/r-$n.txt" && same=$((same+1)) || diff=$((diff+1)); done
echo "seedgen  same $same  differ $diff"; [ "$diff" = 0 ] && [ "$same" -gt 0 ]
