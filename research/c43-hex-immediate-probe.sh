#!/bin/sh
# c43 hex immediate acceptance probe (DRAFT, design only): PRODUCT vs REF on the same tapes, four modes.
# usage: research/c43-hex-immediate-probe.sh PRODUCT REF
# Positive: hex, negative hex, octal and binary immediates, and the 64-bit bounds, must match REF byte for byte
# in -S (text), -c and -o (object bytes), and -run (stdout). Negative: an out-of-range hex literal must be
# refused by both, with empty stdout. Exit 0 only when every case matches; each run is bounded by the caller.
set -u
P=$1; R=$2; T=$(mktemp -d "${TMPDIR:-/tmp}/c43hex.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
fail=0; n=0
tape() { printf '_start:\n  imm r2, %s\n  ret\n' "$1" > "$T/$2.tape"; }
tape 31 dec; tape 0x1f hex; tape -0x10 neghex; tape 0x7fffffffffffffff hi64; tape -0x8000000000000000 lo64
tape 0o17 oct; tape 0b11111 bin; tape 0x10000000000000000 over64
for t in dec hex neghex hi64 lo64 oct bin; do
  for m in S c o run; do
    n=$((n+1)); cp "$T/$t.tape" "$T/in.tape"
    case $m in
      S)   a="-S -o $T/out-p.txt";  b="-S -o $T/out-r.txt" ;;
      c)   a="-c -o $T/out-p.bin";  b="-c -o $T/out-r.bin" ;;
      o)   a="-o $T/out-p.bin";     b="-o $T/out-r.bin" ;;
      run) a="-run";                b="-run" ;;
    esac
    (cd "$T" && sh "$P" $a in.tape > "$T/sp.txt" 2> "$T/ep.txt"); rp=$?
    (cd "$T" && "$R" $b in.tape > "$T/sr.txt" 2> "$T/er.txt"); rr=$?
    if [ "$rp" -ne "$rr" ]; then echo "FAIL $t $m rc product=$rp ref=$rr: $(head -c 100 "$T/ep.txt")"; fail=1; continue; fi
    if [ "$rr" -ne 0 ]; then   # both refused with the same rc: each side must name the refusal (cc ruling)
      [ -s "$T/ep.txt" ] && [ -s "$T/er.txt" ] || { echo "FAIL $t $m refused without a named diagnostic"; fail=1; }
      continue
    fi
    case $m in S) cmp -s "$T/out-p.txt" "$T/out-r.txt" 2>/dev/null || { echo "FAIL $t $m text differs"; fail=1; } ;;
      c|o) cmp -s "$T/out-p.bin" "$T/out-r.bin" 2>/dev/null || { echo "FAIL $t $m bytes differ"; fail=1; } ;;
      run) cmp -s "$T/sp.txt" "$T/sr.txt" || { echo "FAIL $t $m stdout differs"; fail=1; } ;;
    esac
  done
done
cp "$T/over64.tape" "$T/in.tape"
(cd "$T" && sh "$P" -o "$T/x" in.tape > "$T/np.txt" 2>/dev/null); rp=$?
(cd "$T" && "$R" -o "$T/y" in.tape > "$T/nr.txt" 2>/dev/null); rr=$?
if [ "$rp" -eq 0 ] || [ "$rr" -eq 0 ] || [ -s "$T/np.txt" ]; then echo "FAIL over64 must be refused (product=$rp ref=$rr)"; fail=1; fi
echo "c43-hex-immediate-probe: $n cases, fail=$fail"
exit $fail
