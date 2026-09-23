#!/bin/bash
# Shipped-artifact verification. [A-24]
#
# Magic numbers only prove a file is not garbage.  These checks prove the
# artifacts are USABLE: the weight blob reconstructs the same decisions, the
# images are recognised by the platform's own tools, and shipping twice gives
# the same bytes.
set -u
pass=0; fail=0
chk() { if [ "$2" = "$3" ]; then pass=$((pass+1)); printf "  ok   %-34s %s\n" "$1" "$3"
        else fail=$((fail+1)); printf "  FAIL %-34s got '%s' want '%s'\n" "$1" "$3" "$2"; fi; }

echo "== UNS2 weight blob round-trips to the same decisions =="
chk "round-trip mismatches" "0" "$(python3 - <<'PY'
import sys; sys.path.insert(0, '.')
from unisa.__main__ import _built
from unisa.gold import STAGES, ALL
from unisa import uns2
a = _built(); b = uns2.load(uns2.dump(a, STAGES), STAGES)
print(sum(1 for st in ALL for kv in STAGES[st].keys()
          if a[st].predict(kv) != b[st].predict(kv)))
PY
)"

echo "== every target produces a structurally valid image =="
for t in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    n=$(echo "$t" | tr / -)
    python3 -m unisa compile examples/hello.c -o "/tmp/art_$n" --target "$t" \
        --drive built >/dev/null 2>&1
    if command -v file >/dev/null; then
        d=$(file -b "/tmp/art_$n" | cut -c1-34)
    else
        d=$(od -An -tx1 -N4 "/tmp/art_$n" | tr -d ' \n')
    fi
    case "$t:$d" in
        lnx/*:ELF*)        pass=$((pass+1)); printf "  ok   %-14s %s\n" "$t" "$d";;
        osx/*:Mach-O*)     pass=$((pass+1)); printf "  ok   %-14s %s\n" "$t" "$d";;
        win/*:*PE32+*|win/*:MS-DOS*) pass=$((pass+1)); printf "  ok   %-14s %s\n" "$t" "$d";;
        *) fail=$((fail+1)); printf "  FAIL %-14s %s\n" "$t" "$d";;
    esac
done

echo "== shipping is byte-reproducible [D-5] =="
python3 -m unisa ship --out /tmp/art_k1.zip >/dev/null
python3 -m unisa ship --out /tmp/art_k2.zip >/dev/null
python3 -c "
import zipfile,sys
a=zipfile.ZipFile('/tmp/art_k1.zip'); b=zipfile.ZipFile('/tmp/art_k2.zip')
na,nb=sorted(a.namelist()),sorted(b.namelist())
print('same' if na==nb and all(a.read(n)==b.read(n) for n in na) else 'differs')
" > /tmp/art_rep
chk "two ships" "same" "$(cat /tmp/art_rep)"

echo "== kit contents and budgets [B-2] =="
chk "kit members" "4" "$(python3 -c "
import zipfile
z=zipfile.ZipFile('/tmp/art_k1.zip').namelist()
print(sum(1 for p in ('weights/','MANIFEST.json','kernel/','images/')
          if any(n.startswith(p) for n in z)))")"
w=$(python3 -c "
import zipfile; print(len(zipfile.ZipFile('/tmp/art_k1.zip').read('weights/built.uns2')))")
k=$(wc -c < /tmp/art_k1.zip | tr -d ' ')
chk "weights <= 32768 B" "yes" "$([ "$w" -le 32768 ] && echo yes || echo "no ($w)")"
chk "kit <= 65536 B" "yes" "$([ "$k" -le 65536 ] && echo yes || echo "no ($k)")"

echo; echo "artifacts $pass   failed $fail"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]
